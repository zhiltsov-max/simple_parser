if (NOT GTEST_FOUND)
  find_package(GTest QUIET) # try to find system GTest
  if (NOT GTEST_FOUND)
  add_subdirectory(gtest) # fallback to downloaded one
  endif()
endif()

add_executable(unit_tests
  lexer_tests.cpp
  parser_tests.cpp
  )
target_link_libraries(unit_tests
  PRIVATE
    GTest::GTest GTest::Main
    Parser::Parser
  )

add_test(NAME unit_tests COMMAND unit_tests)
