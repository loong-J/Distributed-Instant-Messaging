cmake_minimum_required(VERSION 3.10)
project(AsyncLog)



aux_source_directory(. SRC_LIST)


# 生成日志库 (静态库 libmuduo_log.a)
add_library(AsyncLog STATIC ${SRC_LIST})

target_compile_options(AsyncLog PRIVATE -O2 -Wall)

target_link_libraries(AsyncLog PUBLIC pthread)


