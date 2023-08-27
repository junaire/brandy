cmake_minimum_required(VERSION 3.24)
project(brandy)

set(CMAKE_CXX_STANDARD 20)

# Put binaries and libraries in the same location.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# Always export compile_commands.json for lsp like clangd.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(src)
