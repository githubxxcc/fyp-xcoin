cmake_minimum_required (VERSION 3.5)
project (XCoin)

# C++11
set (CMAKE_CXX_STANDARD 11)

include("${CMAKE_SOURCE_DIR}/cmake/ShadowTools.cmake")

set(OPENSSL_LIB "/home/ricky/.shadow/lib/libssl.so")
set(HEADERS_INCLUDE "${CMAKE_SOURCE_DIR}/include")
find_package(Boost REQUIRED system serialization)

message(STATUS "boost = ${Boost_LIBRARIES}")
message(STATUS "boost = ${Boost_SYSTEM_LIBRARIES}")
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${HEADERS_INCLUDE})

## create and install a shared library that can plug into shadow add_shadow_plugin(shadow-plugin-hello hello.c)
add_shadow_plugin(shadow-plugin-xcoin main.cpp util.cpp util.h block.h block.cpp)
target_link_libraries(shadow-plugin-xcoin -levent -pthread ${OPENSSL_LIB} -lboost_system -lboost_serialization ${HEADERS_INCLUDE})
install(TARGETS shadow-plugin-xcoin DESTINATION plugins)

## create and install an executable that can run outside of shadow
add_executable(xcoin main.cpp util.cpp util.h block.h block.cpp ${HEADERS_INCLUDE})
target_link_libraries(xcoin -levent -pthread -lboost_system -lboost_serialization -lssl -lcrypto)
install(TARGETS xcoin DESTINATION bin)
