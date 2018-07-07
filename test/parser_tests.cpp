#include "gtest/gtest.h"

#include "parser.hxx"

#include <map>
#include <sstream>
#include <string>


using namespace parsing;

TEST(ParserTests, can_create)
{
  std::stringstream ss;

  ASSERT_NO_THROW(Parser{ss});
}

TEST(ParserTests, can_not_parse_empty)
{
  Parser::ParsedTree const expectedTree = {};
  std::string const line = "";
  std::stringstream ss(line);
  Parser parser(ss);

  Parser::ParsingResult const result = parser.parse();

  ASSERT_FALSE(result.m_success);
}

TEST(ParserTests, can_parse_section)
{
  Parser::ParsedTree const expectedTree = {
    {"key", "value"}
  };
  std::string const line = "{ key: \"value\" }";
  std::stringstream ss(line);
  Parser parser(ss);

  Parser::ParsingResult const result = parser.parse();

  ASSERT_TRUE(result.m_success);
  ASSERT_EQ(expectedTree, result.m_tree);
}

TEST(ParserTests, can_parse_nested_section)
{
  Parser::ParsedTree const expectedTree = {
    {"key", ""},
    {"key:key2", "value"}
  };
  std::string const line = "{ key: { key2: \"value\" } }";
  std::stringstream ss(line);
  Parser parser(ss);

  Parser::ParsingResult const result = parser.parse();

  ASSERT_TRUE(result.m_success);
  ASSERT_EQ(expectedTree, result.m_tree);
}

TEST(ParserTests, can_parse_empty_section)
{
  Parser::ParsedTree const expectedTree = {};
  std::string const line = "{ }";
  std::stringstream ss(line);
  Parser parser(ss);

  Parser::ParsingResult const result = parser.parse();

  ASSERT_TRUE(result.m_success);
  ASSERT_EQ(expectedTree, result.m_tree);
}

TEST(ParserTests, can_parse_empty_nested_section)
{
  Parser::ParsedTree const expectedTree = {
    { "key", "" }
  };
  std::string const line = "{ key: { } }";
  std::stringstream ss(line);
  Parser parser(ss);

  Parser::ParsingResult const result = parser.parse();

  ASSERT_TRUE(result.m_success);
  ASSERT_EQ(expectedTree, result.m_tree);
}

TEST(ParserTests, can_parse_not_empty_nested_section)
{
  Parser::ParsedTree const expectedTree = {
    { "key", "" },
    { "key:k2", "v1" },
    { "key:k3", "" }
  };
  std::string const line = "{ key: { k2: \"v1\", k3: \"\" } }";
  std::stringstream ss(line);
  Parser parser(ss);

  Parser::ParsingResult const result = parser.parse();

  ASSERT_TRUE(result.m_success);
  ASSERT_EQ(expectedTree, result.m_tree);
}
