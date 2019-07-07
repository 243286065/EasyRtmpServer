
cmake_minimum_required (VERSION 2.8)
project(demo)
aux_source_directory(. DIR_SRCS)

# 添加子目录
add_subdirectory(src)

# 指定生成目标
add_executable(demo ${DIR_SRCS})
# 添加链接库
target_link_libraries(demo RtmpServer)
