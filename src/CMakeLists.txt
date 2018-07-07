add_library(parser
  parser.cxx
  )
target_include_directories(parser
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<INSTALL_INTERFACE:include>
  )

install(TARGETS parser
  EXPORT ${PROJECT_NAME}Targets
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  )

install(DIRECTORY ../include
  DESTINATION
    include
  )

add_library(Parser::Parser ALIAS parser)
