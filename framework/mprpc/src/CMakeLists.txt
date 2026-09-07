
aux_source_directory(. SRC_LIST)
# set(SRC_LIST 
#     mprpcapplication.cc 
#     mprpcconfig.cc 
#     rpcprovider.cc 
#   )
add_library(mprpc ${SRC_LIST})

target_link_libraries(mprpc mymuduo AsyncLog pthread zookeeper_mt)