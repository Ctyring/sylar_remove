#ifndef __SYLAR_THREAD_H__
#define __SYLAR_THREAD_H__
#include <thread>
#include <functional>
#include <memory>
#include <pthread.h>
namespace sylar
{
    class Thread
    {
    public:
        typedef std::shared_ptr<Thread> ptr;
        Thread(std::function<void()> cb, const std::string &name);
        ~Thread();
        pid_t getId() const { return m_id; }
        const std::string &getName() const { return m_name; };
        void join();
        // 获取自己当前的线程
        static Thread *GetThis();
        static const std::string &GetName();
        static void SetName(const std::string &name);

    private:
        //禁止拷贝
        Thread(const Thread &) = delete;
        //禁止右值引用
        Thread(const Thread &&) = delete;
        //禁止拷贝
        Thread &operator=(const Thread &) = delete;
        static void *run(void *arg);

    private:
        pid_t m_id;
        pthread_t m_thread;
        std::function<void()> m_cb;
        std::string m_name;
    };

}
#endif