cmake_minimum_required (VERSION 3.5)
project (XCoin)

set(XCOIN_INCLUDE_DIR include)
include_directories("${XCOIN_INCLUDE_DIR}")

# Add sources
file(GLOB SOURCES "src/*.cpp")
add_executable(XCoin ${SOURCES})
