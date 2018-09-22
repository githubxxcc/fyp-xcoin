cmake_minimum_required (VERSION 3.5)
project (XCoin)

include("${CMAKE_SOURCE_DIR}/cmake/ShadowTools.cmake")

## create and install a shared library that can plug into shadow add_shadow_plugin(shadow-plugin-hello hello.c)
add_shadow_plugin(shadow-plugin-xcoin main.c)
target_link_libraries(shadow-plugin-xcoin -levent)
install(TARGETS shadow-plugin-xcoin DESTINATION plugins)

## create and install an executable that can run outside of shadow
add_executable(xcoin main.c)
target_link_libraries(xcoin -levent)
install(TARGETS xcoin DESTINATION bin)
