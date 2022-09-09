#include <unistd.h>
#include <iostream>
#include "sylar/sylar.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

int count = 0;
sylar::Mutex s_mutex;

void fun1() {
    SYLAR_LOG_INFO(g_logger)
        << "name: " << sylar::Thread::GetName()
        << " this.name: " << sylar::Thread::GetThis()->getName()
        << " id: " << sylar::GetThreadId()
        << " this.id: " << sylar::Thread::GetThis()->getId();

    for (int i = 0; i < 100000; ++i) {
        sylar::Mutex::Lock lock(
            s_mutex);  // 定义lock对象，类型为sylar::Mutex::Lock，传入互斥量，在构造函数中完成加锁操作，如果该锁已经被持有，那构造lock时就会阻塞，直到锁被释放
        ++count;
    }
}

void fun2() {
    // while (true) {
    SYLAR_LOG_INFO(g_logger) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    // }
}

void fun3() {
    // while (true) {
    SYLAR_LOG_INFO(g_logger) << "========================================";
    // }
}

int main(int argc, char** argv) {
    // sylar::Thread::ptr thr(
    //     new sylar::Thread(&fun2, "name_" + std::to_string(1)));
    // thr->join();
    std::cout << "test_thread" << std::endl;
    SYLAR_LOG_INFO(g_logger) << "thread test begin";
    std::cout << "ok" << std::endl;
    YAML::Node root = YAML::LoadFile("/root/cty/sylar/bin/conf/log.yml");
    sylar::Config::LoadFromYaml(root);

    std::vector<sylar::Thread::ptr> thrs;
    for (int i = 0; i < 2; ++i) {
        sylar::Thread::ptr thr(
            new sylar::Thread(&fun2, "name_" + std::to_string(i * 2)));
        sylar::Thread::ptr thr2(
            new sylar::Thread(&fun3, "name_" + std::to_string(i * 2 + 1)));
        thrs.push_back(thr);
        thrs.push_back(thr2);
    }
    for (size_t i = 0; i < thrs.size(); ++i) {
        thrs[i]->join();
    }
    SYLAR_LOG_INFO(g_logger) << "thread test end";
    SYLAR_LOG_INFO(g_logger) << "count=" << count;
    return 0;
}