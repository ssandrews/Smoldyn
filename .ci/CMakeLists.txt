#
# This is for developers only
#
cmake_minimum_required(VERSION 3.12)
find_program(docker DOCKER_BIN)

add_custom_target(ubuntu 
    COMAMND docker build 
    COMMENT "Building for ubuntu")
