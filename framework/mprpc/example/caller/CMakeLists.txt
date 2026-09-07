set(SRC_LIST calluserservice.cc ../user.pb.cc)

add_executable(userConsumer ${SRC_LIST})

target_link_libraries(userConsumer mprpc protobuf pthread)
