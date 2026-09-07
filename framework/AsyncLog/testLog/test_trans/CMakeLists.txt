

aux_source_directory(. SRC_LIST)


add_executable(tostring ${SRC_LIST})

target_link_libraries(tostring AsyncLog pthread)


