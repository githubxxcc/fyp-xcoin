cmake_minimum_required (VERSION 3.5)
project (XCoin)

# C++11
set (CMAKE_CXX_STANDARD 11)

include("${CMAKE_SOURCE_DIR}/cmake/ShadowTools.cmake")

## create and install a shared library that can plug into shadow add_shadow_plugin(shadow-plugin-hello hello.c)
add_shadow_plugin(shadow-plugin-xcoin main.cpp util.cpp util.h block.h block.cpp)
set(OPENSSL_LIB "/home/ricky/.shadow/lib/libssl.so")
target_link_libraries(shadow-plugin-xcoin -levent -pthread ${OPENSSL_LIB})
install(TARGETS shadow-plugin-xcoin DESTINATION plugins)

## create and install an executable that can run outside of shadow
add_executable(xcoin main.cpp util.cpp util.h block.h block.cpp)
target_link_libraries(xcoin -levent -pthread -lboost_system -lboost_thread -lssl -lcrypto)
install(TARGETS xcoin DESTINATION bin)
