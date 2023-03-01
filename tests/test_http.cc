#include "sylar/http/http.h"
#include "sylar/log.h"
void test() {
    sylar::http::HttpRequest::ptr req(new sylar::http::HttpRequest);
    req->setHeader("host", "www.sylar.top");
    req->setBody("hello world!");
    req->dump(std::cout) << std::endl;
}

void test_response() {
    sylar::http::HttpResponse::ptr resp(new sylar::http::HttpResponse);
    resp->setHeader("X-X", "sylar");
    resp->setBody("hello world!");
    resp->setStatus(sylar::http::HttpStatus::BAD_REQUEST);
    resp->setClose(false);
    // 设置长连接
    resp->setHeader("Connection", "keep-alive");
    resp->dump(std::cout) << std::endl;
}
int main(int argc, char** argv) {
    test_response();
    return 0;
}