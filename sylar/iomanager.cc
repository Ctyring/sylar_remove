#include "iomanager.h"
#include "log.h"
#include "macro.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

IOManager::FdContext::EventContext& IOManager::FdContext::getContext(
    IOManager::Event event) {
    switch (event) {
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            SYLAR_ASSERT2(false, "getContext");
    }
}

void IOManager::FdContext::resetContext(EventContext& ctx) {
    ctx.scheduler = nullptr;
    ctx.fiber.reset();
    ctx.cb = nullptr;
}

void IOManager::FdContext::triggerEvent(IOManager::Event event) {
    // 断言事件是否存在
    SYLAR_ASSERT(events & event);
    // 取出事件
    events = (Event)(events & ~event);
    // 获取上下文
    EventContext& ctx = getContext(event);
    // 执行
    if (ctx.cb) {
        ctx.scheduler->schedule(&ctx.cb);
    } else {
        ctx.scheduler->schedule(&ctx.fiber);
    }
    // 释放调度器
    ctx.scheduler = nullptr;
    return;
}

IOManager::IOManager(size_t threads, bool use_caller, const std::string& name)
    : Scheduler(threads, use_caller, name) {
    // 创建一个 epoll 实例。 返回新实例的 fd。
    m_epfd = epoll_create(5000);
    SYLAR_ASSERT(m_epfd > 0);

    // 创建匿名管道
    int rt = pipe(m_tickleFds);
    SYLAR_ASSERT(!rt);

    // 设置epoll事件并设置为边缘触发模式
    epoll_event event;
    memset(&event, 0, sizeof(epoll_event));
    event.events = EPOLLIN | EPOLLET;
    // 把epoll的fd和匿名管道的读端fd关联起来
    event.data.fd = m_tickleFds[0];

    // 把读端设置为非阻塞
    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
    SYLAR_ASSERT(!rt);

    // 把管道读端以及关注的触发事件添加到epoll实例中
    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
    SYLAR_ASSERT(!rt);

    contextResize(32);

    // 启动调度器
    start();
}

IOManager::~IOManager() {
    stop();
    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    for (size_t i = 0; i < m_fdContexts.size(); ++i) {
        if (m_fdContexts[i]) {
            delete m_fdContexts[i];
        }
    }
}

void IOManager::contextResize(size_t size) {
    m_fdContexts.resize(size);

    for (size_t i = 0; i < m_fdContexts.size(); ++i) {
        if (!m_fdContexts[i]) {
            m_fdContexts[i] = new FdContext;
            m_fdContexts[i]->fd = i;
        }
    }
}

int IOManager::addEvent(int fd, Event event, std::function<void()> cb) {
    FdContext* fd_ctx = nullptr;
    RWMutexType::ReadLock lock(m_mutex);
    // 判断大小，不够就扩容
    if ((int)m_fdContexts.size() > fd) {
        fd_ctx = m_fdContexts[fd];
        lock.unlock();
    } else {
        lock.unlock();
        RWMutexType::WriteLock lock2(m_mutex);
        contextResize(fd * 1.5);
        fd_ctx = m_fdContexts[fd];
    }

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    // 判断事件是否存在
    if (fd_ctx->events & event) {
        SYLAR_LOG_ERROR(g_logger)
            << "addEvent assert fd=" << fd << " event=" << event
            << " fd_ctx.event=" << fd_ctx->events;
        SYLAR_ASSERT(!(fd_ctx->events & event));
    }

    // 判断具体操作是新增还是修改
    int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    // 构造新的事件
    epoll_event epevent;
    epevent.events = EPOLLET | fd_ctx->events | event;
    epevent.data.ptr = fd_ctx;
    // 修改epoll中对该fd的监听情况
    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        SYLAR_LOG_ERROR(g_logger)
            << "epoll_ctl(" << m_epfd << ", " << op << "," << fd << ","
            << epevent.events << "):" << rt << " (" << errno << ") ("
            << strerror(errno) << ")";
        return -1;
    }
    // 构建这个事件的上下文
    // 这个事件进来之后，等待的时间就会+1
    ++m_pendingEventCount;
    fd_ctx->events = (Event)(fd_ctx->events | event);
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    SYLAR_ASSERT(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);

    //设置参数
    event_ctx.scheduler = Scheduler::GetThis();
    if (cb) {
        event_ctx.cb.swap(cb);
    } else {
        event_ctx.fiber = Fiber::GetThis();
        SYLAR_ASSERT2(event_ctx.fiber->getState() == Fiber::EXEC,
                      "state=" << event_ctx.fiber->getState());
    }
    return 0;
}

bool IOManager::delEvent(int fd, Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    if ((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (!(fd_ctx->events & event)) {
        return false;
    }

    Event new_events = (Event)(fd_ctx->events & ~event);
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        SYLAR_LOG_ERROR(g_logger)
            << "epoll_ctl(" << m_epfd << ", " << op << "," << fd << ","
            << epevent.events << "):" << rt << " (" << errno << ") ("
            << strerror(errno) << ")";
        return false;
    }

    --m_pendingEventCount;
    fd_ctx->events = new_events;
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    fd_ctx->resetContext(event_ctx);
    return true;
}

bool IOManager::cancelEvent(int fd, Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    if ((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (!(fd_ctx->events & event)) {
        return false;
    }

    Event new_events = (Event)(fd_ctx->events & ~event);
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        SYLAR_LOG_ERROR(g_logger)
            << "epoll_ctl(" << m_epfd << ", " << op << "," << fd << ","
            << epevent.events << "):" << rt << " (" << errno << ") ("
            << strerror(errno) << ")";
        return false;
    }

    fd_ctx->triggerEvent(event);
    --m_pendingEventCount;
    return true;
}

bool IOManager::cancelAll(int fd) {
    RWMutexType::ReadLock lock(m_mutex);
    if ((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (!fd_ctx->events) {
        return false;
    }

    int op = EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = 0;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        SYLAR_LOG_ERROR(g_logger)
            << "epoll_ctl(" << m_epfd << ", " << op << "," << fd << ","
            << epevent.events << "):" << rt << " (" << errno << ") ("
            << strerror(errno) << ")";
        return false;
    }

    if (fd_ctx->events & READ) {
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
    }
    if (fd_ctx->events & WRITE) {
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
    }

    SYLAR_ASSERT(fd_ctx->events == 0);
    return true;
}

IOManager* IOManager::GetThis() {
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}

void IOManager::tickle() {
    if (!hasIdleThreads()) {
        return;
    }
    int rt = write(m_tickleFds[1], "T", 1);
    SYLAR_ASSERT(rt == 1);
}

bool IOManager::stopping(uint64_t& timeout) {
    timeout = getNextTimer();
    return timeout == ~0ull && m_pendingEventCount == 0 &&
           Scheduler::stopping();
}

bool IOManager::stopping() {
    uint64_t timeout = 0;
    return stopping(timeout);
}

void IOManager::idle() {
    const uint64_t MAX_EVNETS = 256;
    epoll_event* events = new epoll_event[MAX_EVNETS]();
    std::shared_ptr<epoll_event> shared_events(
        events, [](epoll_event* ptr) { delete[] ptr; });

    while (true) {
        uint64_t next_timeout = 0;
        // 判断是否已经完成所有任务，开始停止
        if (stopping()) {
            next_timeout = getNextTimer();
            if (next_timeout == ~0ull) {
                SYLAR_LOG_INFO(g_logger)
                    << "name=" << getName() << " idle stopping exit";
                break;
            }
        }

        int rt = 0;
        do {
            static const int MAX_TIMEOUT = 3000;
            if (next_timeout != ~0ull) {
                next_timeout = (int)next_timeout > MAX_TIMEOUT ? MAX_TIMEOUT
                                                               : next_timeout;
            } else {
                next_timeout = MAX_TIMEOUT;
            }
            // 等待事件，参数events里标注了不阻塞，所以这里会让出CPU
            rt = epoll_wait(m_epfd, events, MAX_EVNETS, (int)next_timeout);
            if (rt < 0 && errno == EINTR) {
            } else {
                // 如果有事件，就跳循环
                break;
            }
        } while (true);
        // 拿到所有要处理的定时器回调函数
        std::vector<std::function<void()>> cbs;
        listExpiredCb(cbs);
        // 统一处理
        if (!cbs.empty()) {
            // SYLAR_LOG_DEBUG(g_logger) << "on timer cbs.size=" << cbs.size();
            schedule(cbs.begin(), cbs.end());
            cbs.clear();
        }

        // 处理完定时器开始处理IO事件
        for (int i = 0; i < rt; ++i) {
            epoll_event& event = events[i];
            // 如果是tickle事件，就读完跳过（这个事件只是为了唤醒epoll_wait）
            if (event.data.fd == m_tickleFds[0]) {
                uint8_t dummy[MAX_EVNETS];
                while (read(m_tickleFds[0], dummy, sizeof(dummy)) > 0) {
                }
                continue;
            }
            // 如果是IO事件，就处理
            FdContext* fd_ctx = (FdContext*)event.data.ptr;
            FdContext::MutexType::Lock lock(fd_ctx->mutex);
            // 如果出现了错误，就把输入和输出都处理一下
            if (event.events & (EPOLLERR | EPOLLHUP)) {
                event.events |= EPOLLIN | EPOLLOUT;
            }
            int real_events = NONE;
            if (event.events & EPOLLIN) {
                real_events |= READ;
            }
            if (event.events & EPOLLOUT) {
                real_events |= WRITE;
            }
            // 如果没有事件，就退出
            if ((fd_ctx->events & real_events) == NONE) {
                continue;
            }
            // 获取剩余事件(看是否都被处理了)
            int left_events = (fd_ctx->events & ~real_events);
            // 如果处理完了就退出
            int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            // 从剩余事件继续放入epoll
            event.events = EPOLLET | left_events;
            // 修改epoll的监听
            int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
            if (rt2) {
                SYLAR_LOG_ERROR(g_logger)
                    << "epoll_ctl(" << m_epfd << ", " << op << "," << fd_ctx->fd
                    << "," << event.events << "):" << rt2 << " (" << errno
                    << ") (" << strerror(errno) << ")";
                continue;
            }
            // 处理事件
            if (fd_ctx->events & READ) {
                fd_ctx->triggerEvent(READ);
                --m_pendingEventCount;
            }
            if (fd_ctx->events & WRITE) {
                fd_ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }
        }
        // 让出协程，处理其他协程，处理完会再切进来(scheduler的run方法)
        // 只有这个协程切出去了，才会去调度其他协程，刚刚其实只是把任务排序管理了一下
        Fiber::ptr cur = Fiber::GetThis();
        auto raw_ptr = cur.get();
        cur.reset();

        raw_ptr->swapOut();
    }
}

void IOManager::onTimerInsertedAtFront() {
    tickle();
}
}  // namespace sylar