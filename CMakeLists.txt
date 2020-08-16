cmake_minimum_required(VERSION 3.16)
project(EpollTest)

add_executable(EpollServer Server.cpp Common.h)
add_executable(MultipleServer MultiplethreadServer.cpp Common.h)
add_executable(EpollClient Client.cpp Common.h)

target_link_libraries(MultipleServer boost_thread boost_system)

