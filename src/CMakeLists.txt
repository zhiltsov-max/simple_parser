add_library(parser
  parser.cxx
  )
target_include_directories(parser
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<INSTALL_INTERFACE:include>
  )
set_property(TARGET parser PROPERTY CXX_STANDARD 14)

install(TARGETS parser
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  )

install(DIRECTORY ../include
  DESTINATION
    include
  )

add_library(parser::parser ALIAS parser)
