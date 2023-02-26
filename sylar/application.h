#pragma once
#include "sylar/http/http_server.h"

namespace sylar {

class Application {
   public:
    Application();

    static Application* GetInstance() { return s_instance; }
    bool init(int argc, char** argv);
    bool run();
    bool getServer(const std::string& type, std::vector<TcpServer::ptr>& svrs);
    void listAllServer(
        std::map<std::string, std::vector<TcpServer::ptr> >& servers);

   private:
    int main(int argc, char** argv);
    int run_fiber();

   private:
    int m_argc = 0;
    char** m_argv = nullptr;

    std::map<std::string, std::vector<TcpServer::ptr> > m_servers;
    IOManager::ptr m_mainIOManager;
    static Application* s_instance;
};

}  // namespace sylar