cmake_minimum_required(VERSION 3.16)
project(LinuxPrograming C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 递归查找所有 .c 源文件
file(GLOB_RECURSE C_SOURCES
    "${CMAKE_SOURCE_DIR}/*.c"
)

# 为每个 .c 文件生成对应的可执行文件
foreach(SOURCE_FILE ${C_SOURCES})
    # 获取不包含扩展名的文件名作为可执行文件名
    get_filename_component(EXEC_NAME ${SOURCE_FILE} NAME_WE)
    add_executable(${EXEC_NAME} ${SOURCE_FILE})
endforeach()
