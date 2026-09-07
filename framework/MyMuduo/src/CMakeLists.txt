
# 加入 -O2 开启标准优化，同时保留 -g 方便排查崩溃堆栈
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2 -std=c++11 -fPIC")

# 定义参与编译的源代码文件 
aux_source_directory(. SRC_LIST)
# 编译生成动态库mymuduo
add_library(mymuduo STATIC ${SRC_LIST})


target_link_libraries(mymuduo AsyncLog)
