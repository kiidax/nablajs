include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${PROJECT_SOURCE_DIR}/third_party/include")
add_executable(nabla
  api.cc
  ast.cc
  builtin.cc
  context.cc
  data.cc
  evalast.cc
  nabla.cc
  parser.cc
  startup.cc
  test.cc
  token.cc
  )

add_custom_command(
    SOURCE token.ll
    COMMAND ${FLEX_EXECUTABLE}
    ARGS -otoken.cc token.ll
    TARGET nabla
    OUTPUTS token.cc)

add_custom_command(
    SOURCE parser.yy
    COMMAND ${BISON_EXECUTABLE}
    ARGS -d -oparser.cc parser.yy
    TARGET nabla
    OUTPUTS parser.cc parser.hh)

add_custom_command(
    SOURCE startup.js
    COMMAND ${SHELL_EXECUTABLE} ./text2c.sh
    ARGS startup.js startup.cc
    TARGET nabla
    OUTPUTS startup.cc text2c.sh)

set(CMAKE_CXX_SOURCE_FILE_EXTENSIONS "cc")
target_link_libraries(nabla ${GC} ${PCRE16})
