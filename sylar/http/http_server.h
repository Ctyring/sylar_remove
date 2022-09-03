#pragma once
#include "http_session.h"
#include "sylar/tcp_server.h"
namespace sylar {
namespace http {
class HttpServer : public TcpServer {
   public:
    typedef std::shared_ptr<HttpServer> ptr;
    HttpServer(bool keepalive = false,
               sylar::IOManager* worker = sylar::IOManager::GetThis(),
               sylar::IOManager* accept_worker = sylar::IOManager::GetThis());

   protected:
    virtual void handleClient(Socket::ptr client) override;

   private:
    bool m_isKeepalive;
    // ServletDispatch::ptr m_dispatch;
};
}  // namespace http
}  // namespace sylar