cmake_minimum_required(VERSION 3.20)
project(balance VERSION 0.3.1 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

file(STRINGS "${CMAKE_SOURCE_DIR}/VERSION" BALANCE_RELEASE_VERSION LIMIT_COUNT 1)
if(NOT BALANCE_RELEASE_VERSION STREQUAL "beta-${PROJECT_VERSION}")
  message(FATAL_ERROR "VERSION must contain beta-${PROJECT_VERSION}; found: ${BALANCE_RELEASE_VERSION}")
endif()

find_package(OpenSSL 3 REQUIRED COMPONENTS Crypto)
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)
find_library(CURSES_LIBRARY NAMES ncursesw ncurses)
if(NOT CURSES_LIBRARY)
  find_file(CURSES_LIBRARY NAMES libncursesw.so.6 libncurses.so.6 PATHS /lib /usr/lib /lib/x86_64-linux-gnu /usr/lib/x86_64-linux-gnu /usr/local/lib)
endif()
if(NOT CURSES_LIBRARY)
  message(FATAL_ERROR "ncursesw/ncurses runtime library not found")
endif()
if(APPLE)
  set(TINFO_LIBRARY "")
else()
  find_library(TINFO_LIBRARY NAMES tinfo)
  if(NOT TINFO_LIBRARY)
    find_file(TINFO_LIBRARY NAMES libtinfo.so.6 PATHS /lib /usr/lib /lib/x86_64-linux-gnu /usr/lib/x86_64-linux-gnu /usr/local/lib)
  endif()
endif()
find_path(CURSES_INCLUDE_DIR NAMES ncursesw/ncurses.h ncurses.h curses.h)

if(APPLE)
  add_compile_options(-D_DARWIN_C_SOURCE=1)
else()
  add_compile_options(-D_POSIX_C_SOURCE=200809L)
endif()
add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wstrict-prototypes -Wno-format-truncation -Wno-format-overflow -Wmissing-prototypes)
include_directories(src compat)
if(CURSES_INCLUDE_DIR)
  include_directories(${CURSES_INCLUDE_DIR})
endif()

set(BALANCE_COMMON
    src/crypto.c
    src/store.c
    src/proto.c
    src/net.c
    src/cloud.c
    src/commands.c
)

add_executable(balance
    src/main.c
    src/ui.c
    src/common.c
    ${BALANCE_COMMON}
)
target_link_libraries(balance PRIVATE OpenSSL::Crypto curl ${CURSES_LIBRARY} ${TINFO_LIBRARY} Threads::Threads m)


add_executable(balance-server src/server.c src/crypto.c)
target_link_libraries(balance-server PRIVATE OpenSSL::Crypto Threads::Threads)

enable_testing()
add_executable(balance-tests tests/test_all.c src/common.c src/crypto.c src/store.c src/proto.c src/net.c src/cloud.c src/commands.c)
target_link_libraries(balance-tests PRIVATE OpenSSL::Crypto curl Threads::Threads)
target_compile_options(balance-tests PRIVATE -UNDEBUG)
add_test(NAME balance-tests COMMAND balance-tests)

install(TARGETS balance balance-server RUNTIME DESTINATION bin)
