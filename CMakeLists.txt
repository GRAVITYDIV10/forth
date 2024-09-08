cmake_minimum_required(VERSION 3.16)

project(forth LANGUAGES C)

add_executable(forth main.c)

set("CMAKE_C_FLAGS" "-O2 -Wno-int-conversion -foptimize-sibling-calls -fstack-usage" CACHE "STRING" "" FORCE)

include(GNUInstallDirs)
install(TARGETS forth
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
