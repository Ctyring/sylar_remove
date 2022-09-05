#pragma once

#include "http.h"
#include "sylar/socket_stream.h"

namespace sylar {
namespace http {

class HttpConnection : public SocketStream {
   public:
    typedef std::shared_ptr<HttpConnection> ptr;
    HttpConnection(Socket::ptr sock, bool owner = true);
    HttpResponse::ptr recvResponse();
    int sendRequest(HttpRequest::ptr req);
};

}  // namespace http
}  // namespace sylar