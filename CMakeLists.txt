cmake_minimum_required(VERSION 3.15)
project(ContactApp LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# writes build/compile_commands.json so the editor knows include paths (e.g. sodium.h)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# libsodium is used for all of the encryption
# macOS/Linux (and MSYS2 on Windows): found through pkg-config
# Windows with vcpkg: found through vcpkg's CMake package
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(SODIUM QUIET IMPORTED_TARGET libsodium)
endif()
if(TARGET PkgConfig::SODIUM)
    set(SODIUM_TARGET PkgConfig::SODIUM)
else()
    find_package(unofficial-sodium CONFIG REQUIRED)
    set(SODIUM_TARGET unofficial-sodium::sodium)
endif()

# curses draws the vim-like interface
if(WIN32)
    # Windows has no curses, so PDCurses (same functions, uses the Windows console) is downloaded and built
    include(FetchContent)
    FetchContent_Declare(pdcurses
        GIT_REPOSITORY https://github.com/wmcbrine/PDCurses.git
        GIT_TAG 3.9)
    FetchContent_MakeAvailable(pdcurses)
    file(GLOB PDCURSES_SOURCES ${pdcurses_SOURCE_DIR}/pdcurses/*.c ${pdcurses_SOURCE_DIR}/wincon/*.c)
    add_library(pdcurses STATIC ${PDCURSES_SOURCES})
    target_include_directories(pdcurses PUBLIC ${pdcurses_SOURCE_DIR} ${pdcurses_SOURCE_DIR}/wincon)
    # wide (Unicode) build: line-drawing characters like ACS_HLINE are written as Unicode (U+2500)
    # instead of code page 437 bytes, which show up as '?' when the console uses UTF-8 or another code page.
    # PUBLIC so our code sees the same chtype layout as the library
    target_compile_definitions(pdcurses PUBLIC PDC_WIDE)
    set(CURSES_TARGET pdcurses)
else()
    # ncurses comes with macOS and Linux (on Debian/Ubuntu: sudo apt install libncurses-dev)
    set(CURSES_NEED_NCURSES TRUE)
    find_package(Curses REQUIRED)
    add_library(curses_lib INTERFACE)
    target_include_directories(curses_lib INTERFACE ${CURSES_INCLUDE_DIRS})
    target_link_libraries(curses_lib INTERFACE ${CURSES_LIBRARIES})
    set(CURSES_TARGET curses_lib)
endif()

add_executable(contact_app main.cpp Contact.cpp Encryption.cpp database.cpp commands.cpp tui.cpp)
target_link_libraries(contact_app PRIVATE ${SODIUM_TARGET} ${CURSES_TARGET})

# automated tests (no curses needed, they test the data structure and encryption directly)
# build and run with: cmake --build build && ctest --test-dir build --output-on-failure
enable_testing()
add_executable(contact_tests tests/tests.cpp Contact.cpp Encryption.cpp database.cpp)
target_link_libraries(contact_tests PRIVATE ${SODIUM_TARGET})
add_test(NAME unit_tests COMMAND contact_tests)
add_test(NAME million_contacts COMMAND contact_tests --large)
