set(SRC_LIST userservice.cc ../user.pb.cc)

add_executable(userProvider ${SRC_LIST})

target_link_libraries(userProvider mprpc mymuduo AsyncLog protobuf pthread)
