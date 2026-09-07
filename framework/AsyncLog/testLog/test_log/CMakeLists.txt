

aux_source_directory(. SRC_LIST)


add_executable(testlog ${SRC_LIST})

target_link_libraries(testlog AsyncLog pthread)


