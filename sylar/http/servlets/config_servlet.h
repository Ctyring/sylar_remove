#ifndef __SYLAR_HTTP_SERVLET_CONFIG_H__
#define __SYLAR_HTTP_SERVLET_CONFIG_H__

#include "sylar/http/servlet.h"
namespace sylar {
namespace http {
class ConfigServlet : public Servlet {
   public:
    ConfigServlet();
    virtual int32_t handle(sylar::http::HttpRequest::ptr request,
                           sylar::http::HttpResponse::ptr response,
                           sylar::http::HttpSession::ptr session) override;
};
}  // namespace http
}  // namespace sylar

#endif