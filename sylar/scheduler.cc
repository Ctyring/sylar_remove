#include "scheduler.h"
#include "hook.h"
#include "log.h"
#include "macro.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static thread_local Scheduler* t_scheduler = nullptr;
static thread_local Fiber* t_scheduler_fiber = nullptr;

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name)
    : m_name(name) {
    SYLAR_ASSERT(threads > 0);

    if (use_caller) {
        // 初始化主协程
        sylar::Fiber::GetThis();
        // 因为当前线程已经初始化过，就去掉当前线程。
        --threads;
        // 每个线程只能有一个协程调度器
        SYLAR_ASSERT(GetThis() == nullptr);
        // 设置当前线程的协程调度器
        t_scheduler = this;
        // 设置当前线程的协程
        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        sylar::Thread::SetName(m_name);
        // 设置调度器所在协程
        t_scheduler_fiber = m_rootFiber.get();
        m_rootThread = sylar::GetThreadId();
        m_threadIds.push_back(m_rootThread);
    } else {
        m_rootThread = -1;
    }
    m_threadCount = threads;
}

Scheduler::~Scheduler() {
    SYLAR_ASSERT(m_stopping);
    if (GetThis() == this) {
        t_scheduler = nullptr;
    }
}

Scheduler* Scheduler::GetThis() {
    return t_scheduler;
}

Fiber* Scheduler::GetMainFiber() {
    return t_scheduler_fiber;
}

void Scheduler::start() {
    MutexType::Lock lock(m_mutex);
    if (!m_stopping) {
        return;
    }
    m_stopping = false;
    SYLAR_ASSERT(m_threads.empty());

    m_threads.resize(m_threadCount);
    for (size_t i = 0; i < m_threadCount; ++i) {
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this),
                                      m_name + "_" + std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());
    }
    lock.unlock();

    // if (m_rootFiber)
    // {
    //     // m_rootFiber->swapIn();
    //     m_rootFiber->call();
    //     SYLAR_LOG_INFO(g_logger) << "call out " << m_rootFiber->getState();
    // }
}

void Scheduler::stop() {
    m_autoStop = true;
    if (m_rootFiber && m_threadCount == 0 &&
        (m_rootFiber->getState() == Fiber::TERM ||
         m_rootFiber->getState() == Fiber::INIT)) {
        SYLAR_LOG_INFO(g_logger) << this << " stopped";
        m_stopping = true;

        if (stopping()) {
            return;
        }
    }

    // bool exit_on_this_fiber = false;
    if (m_rootThread != -1) {
        SYLAR_ASSERT(GetThis() == this);
    } else {
        SYLAR_ASSERT(GetThis() != this);
    }

    m_stopping = true;
    for (size_t i = 0; i < m_threadCount; ++i) {
        tickle();
    }

    if (m_rootFiber) {
        tickle();
    }

    if (m_rootFiber) {
        if (!stopping()) {
            m_rootFiber->call();
        }
    }

    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);
    }

    for (auto& i : thrs) {
        i->join();
    }

    if (stopping()) {
        return;
    }

    // if(exit_on_this_fiber) {
    // }
}

void Scheduler::setThis() {
    t_scheduler = this;
}

void Scheduler::run() {
    // SYLAR_LOG_INFO(g_logger) << "run";
    set_hook_enable(true);
    setThis();
    if (sylar::GetThreadId() != m_rootThread) {
        t_scheduler_fiber = Fiber::GetThis().get();
    }

    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    Fiber::ptr cb_fiber;

    FiberAndThread ft;
    while (true) {
        ft.reset();
        bool tickle_me = false;
        bool is_active = false;
        {
            MutexType::Lock lock(m_mutex);
            auto it = m_fibers.begin();
            // 遍历所有协程
            while (it != m_fibers.end()) {
                // 如果是别的线程的协程
                if (it->thread != -1 && it->thread != sylar::GetThreadId()) {
                    ++it;
                    tickle_me = true;
                    continue;
                }

                SYLAR_ASSERT(it->fiber || it->cb);
                // 如果协程是执行中
                if (it->fiber && it->fiber->getState() == Fiber::EXEC) {
                    ++it;
                    continue;
                }

                // 到这里说明找到了合适的协程，将其取出
                ft = *it;
                m_fibers.erase(it++);
                // 活跃线程数量+1
                ++m_activeThreadCount;
                is_active = true;
                break;
            }
            tickle_me |= it != m_fibers.end();
        }
        // 如果当前线程没有协程，且有其他线程有协程，那么就唤醒其他线程
        if (tickle_me) {
            tickle();
        }

        // 判断协程状态是否合适
        if (ft.fiber && (ft.fiber->getState() != Fiber::TERM &&
                         ft.fiber->getState() != Fiber::EXCEPT)) {
            // 如果协程状态合适，就切进来执行任务
            ft.fiber->swapIn();
            // 协程切出，活跃线程数量-1
            --m_activeThreadCount;

            // 这时，协程有可能是切换到后台并且是可执行状态(实际上目前代码没有这种情况)
            if (ft.fiber->getState() == Fiber::READY) {
                // 把协程加入调度
                schedule(ft.fiber);
            }
            // 如果协程不是结束或者异常了，因为被切出了，所以状态改成等待。
            else if (ft.fiber->getState() != Fiber::TERM &&
                     ft.fiber->getState() != Fiber::EXCEPT) {
                ft.fiber->m_state = Fiber::HOLD;
            }
            // 把数据释放了。
            ft.reset();
        }
        // 如果只有回调函数，就需要调度器来创建协程执行任务。
        else if (ft.cb) {
            if (cb_fiber) {
                // 重置协程
                cb_fiber->reset(ft.cb);
            } else {
                // 创建协程
                cb_fiber.reset(new Fiber(ft.cb));
            }
            // 这部分同上。
            ft.reset();
            cb_fiber->swapIn();
            --m_activeThreadCount;
            if (cb_fiber->getState() == Fiber::READY) {
                schedule(cb_fiber);
                cb_fiber.reset();
            } else if (cb_fiber->getState() == Fiber::EXCEPT ||
                       cb_fiber->getState() == Fiber::TERM) {
                cb_fiber->reset(nullptr);
            } else {  // if(cb_fiber->getState() != Fiber::TERM) {
                cb_fiber->m_state = Fiber::HOLD;
                cb_fiber.reset();
            }
        } else {
            // 检查调度状态，如果是拿到了无效的协程数据，就继续调度。
            if (is_active) {
                --m_activeThreadCount;
                continue;
            }
            // 如果是真的没任务了，就切换到idle协程。
            if (idle_fiber->getState() == Fiber::TERM) {
                // 如果连idle协程都退出了，就结束了，退出死循环
                SYLAR_LOG_INFO(g_logger) << "idle fiber term";
                break;
            }

            ++m_idleThreadCount;
            idle_fiber->swapIn();
            --m_idleThreadCount;
            if (idle_fiber->getState() != Fiber::TERM &&
                idle_fiber->getState() != Fiber::EXCEPT) {
                idle_fiber->m_state = Fiber::HOLD;
            }
        }
    }
}

void Scheduler::tickle() {
    SYLAR_LOG_INFO(g_logger) << "tickle";
}

bool Scheduler::stopping() {
    MutexType::Lock lock(m_mutex);
    return m_autoStop && m_stopping && m_fibers.empty() &&
           m_activeThreadCount == 0;
}

void Scheduler::idle() {
    SYLAR_LOG_INFO(g_logger) << "idle";
    while (!stopping()) {
        sylar::Fiber::YieldToHold();
    }
}

void Scheduler::switchTo(int thread) {
    SYLAR_ASSERT(Scheduler::GetThis() != nullptr);
    if (Scheduler::GetThis() == this) {
        if (thread == -1 || thread == sylar::GetThreadId()) {
            return;
        }
    }
    schedule(Fiber::GetThis(), thread);
    Fiber::YieldToHold();
}

std::ostream& Scheduler::dump(std::ostream& os) {
    os << "[Scheduler name=" << m_name << " size=" << m_threadCount
       << " active_count=" << m_activeThreadCount
       << " idle_count=" << m_idleThreadCount << " stopping=" << m_stopping
       << " ]" << std::endl
       << "    ";
    for (size_t i = 0; i < m_threadIds.size(); ++i) {
        if (i) {
            os << ", ";
        }
        os << m_threadIds[i];
    }
    return os;
}

SchedulerSwitcher::SchedulerSwitcher(Scheduler* target) {
    m_caller = Scheduler::GetThis();
    if (target) {
        target->switchTo();
    }
}

SchedulerSwitcher::~SchedulerSwitcher() {
    if (m_caller) {
        m_caller->switchTo();
    }
}

}  // namespace sylar