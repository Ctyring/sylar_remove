#include <fstream>
#include <iostream>
#include "sylar/http/http_connection.h"
#include "sylar/iomanager.h"
#include "sylar/log.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void testNormal() {
    sylar::Address::ptr addr =
        sylar::Address::LookupAnyIPAddress("www.sylar.top:80");
    if (!addr) {
        SYLAR_LOG_INFO(g_logger) << "get addr error";
        return;
    }

    sylar::Socket::ptr sock = sylar::Socket::CreateTCP(addr);
    bool rt = sock->connect(addr);
    if (!rt) {
        SYLAR_LOG_INFO(g_logger) << "connect " << *addr << " failed";
        return;
    }

    sylar::http::HttpConnection::ptr conn(
        new sylar::http::HttpConnection(sock));
    sylar::http::HttpRequest::ptr req(new sylar::http::HttpRequest);
    req->setPath("/blog/");
    req->setHeader("host", "www.sylar.top");
    SYLAR_LOG_INFO(g_logger) << "req:" << std::endl << *req;

    conn->sendRequest(req);
    auto rsp = conn->recvResponse();

    if (!rsp) {
        SYLAR_LOG_INFO(g_logger) << "recv response error";
        return;
    }
    SYLAR_LOG_INFO(g_logger) << "rsp:" << std::endl << *rsp;

    std::ofstream ofs("rsp.dat");
    ofs << *rsp;
}

void testHttp() {
    auto r =
        sylar::http::HttpConnection::DoGet("http://www.sylar.top/blog/", 300);
    SYLAR_LOG_INFO(g_logger)
        << "result=" << r->result << " error=" << r->error
        << " rsp=" << (r->response ? r->response->toString() : "");
}

void test_pool() {
    sylar::http::HttpConnectionPool::ptr pool(
        new sylar::http::HttpConnectionPool("www.sylar.top", "", 80, false, 10,
                                            1000 * 30, 5));

    sylar::IOManager::GetThis()->addTimer(
        1000,
        [pool]() {
            auto r = pool->doGet("/", 300);
            SYLAR_LOG_INFO(g_logger) << r->toString();
        },
        true);
}

void test_https() {
    auto r = sylar::http::HttpConnection::DoGet("https://www.baidu.com/", 300);
    SYLAR_LOG_INFO(g_logger)
        << "result=" << r->result << " error=" << r->error
        << " rsp=" << (r->response ? r->response->toString() : "");

    // sylar::http::HttpConnectionPool::ptr pool(new
    // sylar::http::HttpConnectionPool(
    //             "www.baidu.com", "", 443, true, 10, 1000 * 30, 5));
    auto pool = sylar::http::HttpConnectionPool::Create("https://www.baidu.com",
                                                        "", 10, 1000 * 30, 5);

    sylar::IOManager::GetThis()->addTimer(
        1000,
        [pool]() {
            auto r = pool->doGet("/", 300);
            SYLAR_LOG_INFO(g_logger) << r->toString();
        },
        true);
}

void run() {
    // testNormal();
    // SYLAR_LOG_INFO(g_logger) << "=========================";
    // testHttp();
    test_pool();
}

int main(int argc, char** argv) {
    sylar::IOManager iom(2);
    iom.schedule(test_https);
    return 0;
}