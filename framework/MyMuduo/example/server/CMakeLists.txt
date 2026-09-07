# 设置调试信息 以及 启动C++11语言标准
# 添加 -O2 或 -O3 进行优化
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2 -std=c++11 -fPIC")

# 定义参与编译的源代码文件 
aux_source_directory(. SRC_LIST)

add_executable(echoServer ${SRC_LIST})

target_link_libraries(echoServer mymuduo AsyncLog pthread)


