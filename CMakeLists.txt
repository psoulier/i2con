cmake_minimum_required(VERSION 3.5.2 FATAL_ERROR)

project(i2con)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#include(CTest)
#include(GTestCMake/GTest.cmake)
# Was using "~" for my home directory, but CMake decided to stop liking that 
# at some point.  Using the "HOME" environment variable seems to be more 
# palatable for CMake.
#googletest_setup(URL "$ENV{HOME}/Projects/googletest/googletest-master.zip")


set(SERVER_SOURCE 
	"i2conserver.cpp")

set(CLIENT_SOURCE
	"i2conclient.cpp" "i2conclient.hpp")




# Add test executable target
add_executable(i2consrv EXCLUDE_FROM_ALL ${SERVER_SOURCE})

add_executable(i2contest "test_client.cpp" ${CLIENT_SOURCE})
