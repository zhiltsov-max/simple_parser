#include <iostream>
#include <exception>
#include <map>
#include <string>


namespace parsing {

//
// Implements grammar matching:
//
// escape = '\'
// escaped = escape ( 'r' | 'n' | '\' | 'x' [0-9]{4} )
// unescaped = [^\\x00-\x20]
// char = escaped | unescaped
// value = '"' char* '"'
//
// key = [a-zA-Z0-9_]+
// entry = entry_key ':' ( value | section )
// entries = entry ( ',' entry )?
// section = '{' ( entries )? '}'
// S = (UTF-8 BOM)? section
//
// Sample:
//
// {
//   x: "5",
//   55y : {
//     _z7: "sdf\n3443\r"
//   },
//   qq :"hello"
// }
//

enum class TokenKind : int {
  Unknown = 0,
  Key,
  Value,
  SectionBegin,
  SectionEnd,
  EntrySeparator,
  KeyValueSeparator,

  ParseEnd,
  ParseError,

  _count // keep last
};

class Token {
public:
  using ValueType = std::string;

  Token();
  Token(TokenKind kind, std::string const& value);

  TokenKind const& getKind() const;
  ValueType const& getText() const;

  operator bool() const;

  friend bool operator == (std::string const& a, Token const& b);
  friend bool operator == (Token const& a, std::string const& b);

  friend bool operator == (Token const& a, TokenKind const& b);
  friend bool operator == (TokenKind const& a, Token const& b);

private:
  TokenKind m_kind;
  ValueType m_value;
};

class Lexer {
public:
  Lexer(std::istream& is);

  Token const& getCurrent();
  Token const& getNext();

  bool isFinished() const;

  std::istream::pos_type getPosition() const;

private:
  class impl;

  char getChar();
  char peekChar() const;

  std::istream& m_stream;
  Token m_lastToken;
};


enum class ParsingErrorKind {
  UnexpectedTokenReceived,
  UnexpectedDataEnd
};

struct ParsingError {
  ParsingErrorKind m_kind;
  std::istream::pos_type m_position;
};

class Parser {
public:
  using Key = std::string;
  using Value = std::string;
  using ParsedTree = std::map<Key, Value>;

  // TODO: use variant or something similar here
  struct ParsingResult {
    bool m_success;

    ParsedTree m_tree;
    ParsingError m_error;
  };

  Parser(std::istream& is);

  ParsingResult parse();

  static constexpr char s_categorySeparator = ':';

private:
  class impl;

  Lexer m_lexer;
};

} // namespace parsing
