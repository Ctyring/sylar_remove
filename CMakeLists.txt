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
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++17 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")

include_directories(.)
include_directories(/yaml-cpp/include)
link_directories(/yaml-cpp/lib)

message("***", ${YAMLCPP})

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
	)

add_library(sylar SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(sylar) #__FILE__

find_library(YAMLCPP yaml-cpp)
find_library(PTHREAD pthread)

set(LIBS
        sylar
		dl
        pthread
        yaml-cpp
		)

message("***", ${LIBS})

add_executable(test tests/test.cc)
add_dependencies(test sylar)
force_redefine_file_macro_for_sources(test) #__FILE__
target_link_libraries(test ${LIBS})

add_executable(test_config tests/test_config.cc)
add_dependencies(test_config sylar)
force_redefine_file_macro_for_sources(test_config) #__FILE__
target_link_libraries(test_config ${LIBS})

add_executable(test_thread tests/test_thread.cc)
add_dependencies(test_thread sylar)
force_redefine_file_macro_for_sources(test_thread) #__FILE__
target_link_libraries(test_thread ${LIBS})

add_executable(test_util tests/test_util.cc)
add_dependencies(test_util sylar)
force_redefine_file_macro_for_sources(test_util) #__FILE__
target_link_libraries(test_util ${LIBS})

add_executable(test_fiber tests/test_fiber.cc)
add_dependencies(test_fiber sylar)
force_redefine_file_macro_for_sources(test_fiber) #__FILE__
target_link_libraries(test_fiber ${LIBS})

add_executable(test_scheduler tests/test_scheduler.cc)
add_dependencies(test_scheduler sylar)
force_redefine_file_macro_for_sources(test_scheduler) #__FILE__
target_link_libraries(test_scheduler ${LIBS})

add_executable(test_iomanager tests/test_iomanager.cc)
add_dependencies(test_iomanager sylar)
force_redefine_file_macro_for_sources(test_iomanager) #__FILE__
target_link_libraries(test_iomanager ${LIBS})

add_executable(test_hook tests/test_hook.cc)
add_dependencies(test_hook sylar)
force_redefine_file_macro_for_sources(test_hook) #__FILE__
target_link_libraries(test_hook ${LIBS})

add_executable(test_address tests/test_address.cc)
add_dependencies(test_address sylar)
force_redefine_file_macro_for_sources(test_address) #__FILE__
target_link_libraries(test_address ${LIBS})

add_executable(test_socket tests/test_socket.cc)
add_dependencies(test_socket sylar)
force_redefine_file_macro_for_sources(test_socket) #__FILE__
target_link_libraries(test_socket ${LIBS})

add_executable(test_bytearray tests/test_bytearray.cc)
add_dependencies(test_bytearray sylar)
force_redefine_file_macro_for_sources(test_bytearray) #__FILE__
target_link_libraries(test_bytearray ${LIBS})

add_executable(test_http tests/test_http.cc)
add_dependencies(test_http sylar)
force_redefine_file_macro_for_sources(test_http) #__FILE__
target_link_libraries(test_http ${LIBS})

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)