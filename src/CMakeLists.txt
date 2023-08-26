SET(SOURCES
  basic_block.cpp
  function.cpp
  cfg.cpp
  dom.cpp
  ssa.cpp
  context.cpp
  die.cpp
  cse.cpp
  main.cpp
)

add_executable(
  brandy
  ${SOURCES}
)

target_include_directories(
  brandy
  PUBLIC
  ${PROJECT_SOURCE_DIR}/include
)
