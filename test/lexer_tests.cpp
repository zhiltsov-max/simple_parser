#include "gtest/gtest.h"

#include "parser.hxx"

#include <sstream>
#include <string>
#include <vector>


using namespace parsing;

TEST(LexerTests, can_create)
{
  std::stringstream ss;

  ASSERT_NO_THROW(Lexer lexer(ss));
}

TEST(LexerTests, can_parse_eof)
{
  std::stringstream ss;
  Lexer lexer(ss);

  Token token = lexer.getCurrent();

  ASSERT_TRUE(TokenKind::ParseEnd == token.getKind());
}

TEST(LexerTests, can_parse_key_with_adjacent_separator)
{
  std::string const keyName = "key";
  std::string const line = keyName + ":";
  std::stringstream ss(line);
  Lexer lexer(ss);

  Token token = lexer.getCurrent();

  ASSERT_TRUE(TokenKind::Key == token.getKind());
  EXPECT_EQ(keyName, token.getText());
}

TEST(LexerTests, can_parse_key_with_separated_separator)
{
  std::string const keyName = "key";
  std::string const line = keyName + "   :";
  std::stringstream ss(line);
  Lexer lexer(ss);

  Token token = lexer.getCurrent();

  ASSERT_TRUE(TokenKind::Key == token.getKind());
  EXPECT_EQ(keyName, token.getText());
}

TEST(LexerTests, can_parse_key_with_whitespace_before)
{
  std::string const keyName = "key";
  std::string const line = "   " + keyName + ":";
  std::stringstream ss(line);
  Lexer lexer(ss);

  Token token = lexer.getCurrent();

  ASSERT_TRUE(TokenKind::Key == token.getKind());
  EXPECT_EQ(keyName, token.getText());
}

TEST(LexerTests, can_not_parse_key_with_control_char)
{
  constexpr char keyName[] = "k\0x05ey:";
  std::stringstream ss(keyName);
  Lexer lexer(ss);

  Token token = lexer.getCurrent();

  ASSERT_TRUE(TokenKind::ParseError == token.getKind());
}

TEST(LexerTests, can_parse_value_ansi)
{
  std::string const value = "value";
  std::string const line = "\"" + value + "\"";
  std::stringstream ss(line);
  Lexer lexer(ss);

  Token token = lexer.getCurrent();

  ASSERT_TRUE(TokenKind::Value == token.getKind());
  EXPECT_EQ(value, token.getText());
}

TEST(LexerTests, can_parse_value_with_whitespaces_before_and_after)
{
  std::string const value = "value";
  std::string const line = "   \"" + value + "\"  ";
  std::stringstream ss(line);
  Lexer lexer(ss);

  Token token = lexer.getCurrent();

  ASSERT_TRUE(TokenKind::Value == token.getKind());
  EXPECT_EQ(value, token.getText());
}

TEST(LexerTests, can_parse_value_utf8)
{
  std::string const value = u8"строка";
  std::string const line = "\"" + value + "\"";
  std::stringstream ss(line);
  Lexer lexer(ss);

  Token token = lexer.getCurrent();

  ASSERT_TRUE(TokenKind::Value == token.getKind());
  EXPECT_EQ(value, token.getText());
}

TEST(LexerTests, can_parse_value_with_escaped_chars)
{
  std::string const escapedValue = "\\r \\n \\\\ \\x1234";
  std::string const unescapedValue = u8"\r \n \\ \U00001234";
  std::string const line = "\"" + escapedValue + "\"";
  std::stringstream ss(line);
  Lexer lexer(ss);

  Token token = lexer.getCurrent();

  ASSERT_TRUE(TokenKind::Value == token.getKind());
  EXPECT_EQ(unescapedValue, token.getText());
}

TEST(LexerTests, can_not_parse_value_with_too_short_escape_sequence)
{
  std::string const escapedValue = "\\x123";
  std::string const line = "\"" + escapedValue + "\"";
  std::stringstream ss(line);
  Lexer lexer(ss);

  Token token = lexer.getCurrent();

  ASSERT_TRUE(TokenKind::ParseError == token.getKind());
}

TEST(LexerTests, can_parse_value_with_surrogate_pair)
{
  std::string const escapedValue = "\\xDBFF\\xDFFF";
  std::string const unescapedValue = u8"\U0010FFFF";
  std::string const line = "\"" + escapedValue + "\"";
  std::stringstream ss(line);
  Lexer lexer(ss);

  Token token = lexer.getCurrent();

  ASSERT_TRUE(TokenKind::Value == token.getKind());
  EXPECT_EQ(unescapedValue, token.getText());
}

TEST(LexerTests, can_not_parse_value_with_wrong_surrogate_pair)
{
  std::string const escapedValue = "\\xd890\\xd000";
  std::string const line = "\"" + escapedValue + "\"";
  std::stringstream ss(line);
  Lexer lexer(ss);

  Token token = lexer.getCurrent();

  ASSERT_TRUE(TokenKind::ParseError == token.getKind());
}

TEST(LexerTests, can_not_parse_value_with_control_chars)
{
  std::string const value = u8"\x02qq\x15";
  std::string const line = "\"" + value + "\"";
  std::stringstream ss(line);
  Lexer lexer(ss);

  Token token = lexer.getCurrent();

  ASSERT_TRUE(TokenKind::ParseError == token.getKind());
}

TEST(LexerTests, can_parse_utf8_bom)
{
  std::string const value = "vqa";
  std::string const line = "\xEF\xBB\xBF" "\"" + value + "\"";
  std::stringstream ss(line);
  Lexer lexer(ss);

  Token token = lexer.getCurrent();

  ASSERT_TRUE(TokenKind::Value == token.getKind());
  EXPECT_EQ(value, token.getText());
}

TEST(LexerTests, can_not_parse_wrong_utf8_bom)
{
  std::string const value = "vqa";
  std::string const line = "\xEF\xAA\xAA" "\"" + value + "\"";
  std::stringstream ss(line);
  Lexer lexer(ss);

  Token token = lexer.getCurrent();

  ASSERT_TRUE(TokenKind::ParseError == token.getKind());
}

TEST(LexerTests, can_parse_section_begin)
{
  std::string const line = "{";
  std::stringstream ss(line);
  Lexer lexer(ss);

  Token token = lexer.getCurrent();

  ASSERT_TRUE(TokenKind::SectionBegin == token.getKind());
}

TEST(LexerTests, can_parse_section_end)
{
  std::string const line = "}";
  std::stringstream ss(line);
  Lexer lexer(ss);

  Token token = lexer.getCurrent();

  ASSERT_TRUE(TokenKind::SectionEnd == token.getKind());
}

TEST(LexerTests, can_parse_key_value_separator)
{
  std::string const line = ":";
  std::stringstream ss(line);
  Lexer lexer(ss);

  Token token = lexer.getCurrent();

  ASSERT_TRUE(TokenKind::KeyValueSeparator == token.getKind());
}

TEST(LexerTests, can_parse_entry_separator)
{
  std::string const line = ",";
  std::stringstream ss(line);
  Lexer lexer(ss);

  Token token = lexer.getCurrent();

  ASSERT_TRUE(TokenKind::EntrySeparator == token.getKind());
}
