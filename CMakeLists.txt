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
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")

include_directories(.)
include_directories(/yaml-cpp/include)
link_directories(/yaml-cpp/lib)

message("***", ${YAMLCPP})

set(LIB_SRC 
	sylar/log.cc
	sylar/util.cc
	sylar/config.cc
	sylar/thread.cc
	)

add_library(sylar SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(sylar) #__FILE__

find_library(YAMLCPP yaml-cpp)
find_library(PTHREAD pthread)

set(LIBS
        sylar
        pthread
        yaml-cpp)

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

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)