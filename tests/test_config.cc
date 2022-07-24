#include "sylar/config.h"
#include "sylar/log.h"

sylar::ConfigVar<int>::ptr g_int_value_config = sylar::Config::Lookup("system.prot", (int)8080, "system port");

sylar::ConfigVar<float>::ptr g_float_value_config =
    sylar::Config::Lookup("system.value", (float)10.2f, "system value");

int main(int argc, char const *arfv[])
{
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_float_value_config->toString();
    return 0;
}