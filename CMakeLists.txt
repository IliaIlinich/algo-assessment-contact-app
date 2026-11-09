cmake_minimum_required(VERSION 3.15)
project(ContactApp LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# writes build/compile_commands.json so the editor knows include paths (e.g. sodium.h)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# libsodium is found through pkg-config and is used for all of the encryption
find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED IMPORTED_TARGET libsodium)

add_executable(contact_app main.cpp Contact.cpp Encryption.cpp database.cpp)
target_link_libraries(contact_app PRIVATE PkgConfig::SODIUM)
