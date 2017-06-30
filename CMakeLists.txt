cmake_minimum_required(VERSION 3.7)
project(Scheduling_Web_Server)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        makefile
        network.c
        network.h
        sws.c
        test1.in)

add_executable(Scheduling_Web_Server ${SOURCE_FILES})