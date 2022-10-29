cmake_miniMum_required(VERSION 3.0)

project(sylar)
include (cmake/utils.cmake)
set(CMAKE_VERBOSE_MAKEFILE ON)

# 指定编译选项
# -rdynamic 将所有符号加入符号表，便于使用dlopen或者backtrace追踪到符号
# -Werror 不区分警告和错误，遇到任何警告都停止编译
# -Wall 开启大部分警告提示
# -Wno-unused-funtion 不要警告未使用函数
# -Wno-deprecated 不要警告过时特性
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++17 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

include_directories(.)
include_directories(/yaml-cpp/include)
include_directories(/usr/local/include)
link_directories(/yaml-cpp/lib)
link_directories(/root/cty/sylar/include)

option(TEST "ON for complile test" OFF)

message("***", ${YAMLCPP})

find_package(Protobuf)
if(Protobuf_FOUND)
include_directories(${Protobuf_INCLUDE_DIRS})
endif()

find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()

find_package(ZLIB REQUIRED)

if(ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIR})
endif()

set(LIB_SRC 
	sylar/config.cc
	sylar/fiber.cc
	sylar/hook.cc
	sylar/fd_manager.cc
	sylar/iomanager.cc
	sylar/log.cc
	sylar/scheduler.cc
	sylar/timer.cc
	sylar/thread.cc
	sylar/util.cc
	sylar/address.cc
	sylar/socket.cc
	sylar/bytearray.cc
	sylar/http/http.cc
	sylar/http/http_parser.cc
    sylar/http/http11_parser.rl.cc
    sylar/http/httpclient_parser.rl.cc
	sylar/tcp_server.cc
	sylar/streams/socket_stream.cc
    sylar/stream.cc
	sylar/http/http_session.cc
	sylar/http/http_server.cc
	sylar/http/servlet.cc
	sylar/http/http_connection.cc
	sylar/daemon.cc
	sylar/env.cc
	sylar/http/ws_connection.cc
    sylar/http/ws_session.cc
    sylar/http/ws_server.cc
    sylar/http/ws_servlet.cc
	sylar/util/hash_util.cc
	sylar/application.cc
	sylar/util/time.cc
	sylar/worker.cc
	sylar/mutex.cc
	sylar/module.cc
	sylar/library.cc
	sylar/util/json_util.cc
	sylar/streams/zlib_stream.cc
	sylar/streams/async_socket_stream.cc
	sylar/protocol.cc
    sylar/rock/rock_protocol.cc
	sylar/util/crypto_util.cc
	sylar/rock/rock_server.cc
	sylar/db/sqlite3.cc
	sylar/streams/load_balance.cc
	sylar/rock/rock_stream.cc
	)

ragelmaker(sylar/http/http11_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/sylar/http)
ragelmaker(sylar/http/httpclient_parser.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/sylar/http)
ragelmaker(sylar/uri.rl LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/sylar)

# protobufmaker(sylar/test.proto LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(sylar SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(sylar)

find_library(YAMLCPP yaml-cpp)
find_library(PTHREAD pthread)

set(LIBS
        sylar
		dl
        pthread
        yaml-cpp
		${PROTOBUF_LIBRARIES}
		${OPENSSL_LIBRARIES}
		libjsoncpp_static.a
		${ZLIB_LIBRARIES}
		sqlite3
		)

message("***", ${LIBS})
if(TEST)
sylar_add_executable(test1 "tests/test.cc" sylar "${LIBS}")
sylar_add_executable(test_config "tests/test_config.cc" sylar "${LIBS}")
sylar_add_executable(test_thread "tests/test_thread.cc" sylar "${LIBS}")
sylar_add_executable(test_util "tests/test_util.cc" sylar "${LIBS}")
sylar_add_executable(test_fiber "tests/test_fiber.cc" sylar "${LIBS}")
sylar_add_executable(test_scheduler "tests/test_scheduler.cc" sylar "${LIBS}")
sylar_add_executable(test_iomanager "tests/test_iomanager.cc" sylar "${LIBS}")
sylar_add_executable(test_hook "tests/test_hook.cc" sylar "${LIBS}")
sylar_add_executable(test_address "tests/test_address.cc" sylar "${LIBS}")
sylar_add_executable(test_socket "tests/test_socket.cc" sylar "${LIBS}")
sylar_add_executable(test_bytearray "tests/test_bytearray.cc" sylar "${LIBS}")
sylar_add_executable(test_http "tests/test_http.cc" sylar "${LIBS}")
sylar_add_executable(test_http_parser "tests/test_http_parser.cc" sylar "${LIBS}")
sylar_add_executable(test_tcp_server "tests/test_tcp_server.cc" sylar "${LIBS}")
sylar_add_executable(echo_server "examples/echo_server.cc" sylar "${LIBS}")
sylar_add_executable(test_http_server "tests/test_http_server.cc" sylar "${LIBS}")
# sylar_add_executable(test_http_connection "tests/test_http_connection.cc" sylar "${LIBS}")
sylar_add_executable(test_uri "tests/test_uri.cc" sylar "${LIBS}")
sylar_add_executable(my_http_server "samples/my_http_server.cc" sylar "${LIBS}")
sylar_add_executable(test_daemon "tests/test_daemon.cc" sylar "${LIBS}")
sylar_add_executable(test_env "tests/test_env.cc" sylar "${LIBS}")
sylar_add_executable(test_ws_server "tests/test_ws_server.cc" sylar "${LIBS}")
sylar_add_executable(test_ws_client "tests/test_ws_client.cc" sylar "${LIBS}")
sylar_add_executable(test_application "tests/test_application.cc" sylar "${LIBS}")

sylar_add_executable(test_http_connection "tests/test_http_connection.cc" sylar "${LIBS}")
sylar_add_executable(test_lru "tests/test_lru.cc" sylar "${LIBS}")
sylar_add_executable(test_timed_cache "tests/test_timed_cache.cc" sylar "${LIBS}")
sylar_add_executable(test_timed_lru_cache "tests/test_timed_lru_cache.cc" sylar "${LIBS}")
sylar_add_executable(test_zlib_stream "tests/test_zlib_stream.cc" sylar "${LIBS}")
endif()
sylar_add_executable(test_crypto "tests/test_crypto.cc" sylar "${LIBS}")

sylar_add_executable(test_sqlite3 "tests/test_sqlite3.cc" sylar "${LIBS}")
sylar_add_executable(test_rock "tests/test_rock.cc" sylar "${LIBS}")

add_library(test_module SHARED tests/test_module.cc)
sylar_add_executable(bin_sylar "sylar/main.cc" sylar "${LIBS}")
set_target_properties(bin_sylar PROPERTIES OUTPUT_NAME "sylar")

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)