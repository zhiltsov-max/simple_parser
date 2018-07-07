#include "parser.hxx"

#include <algorithm>
#include <array>
#include <exception>
#include <iostream>
#include <iterator>
#include <locale>
#include <map>
#include <numeric>
#include <stack>
#include <string>
#include <vector>


namespace parsing {

class Exception : public std::exception {
public:
  Exception(std::string const& message = "");

  char const* what() const noexcept override;
protected:
  std::string m_message;
};

Exception::Exception(std::string const& message)
  : m_message(message)
{}

char const* Exception::what() const noexcept
{
  return m_message.c_str();
}


Token::Token()
  : m_kind(TokenKind::Unknown)
  , m_value()
{}

Token::Token(TokenKind kind, std::string const& value)
  : m_kind(kind)
  , m_value(value)
{}

Token::operator bool() const
{
  return (m_kind != TokenKind::Unknown);
}

TokenKind const& Token::getKind() const
{
  return m_kind;
}

Token::ValueType const& Token::getText() const
{
  return m_value;
}

bool operator == (std::string const& a, Token const& b)
{
  return (b.m_kind == TokenKind::Value) && (a == b.m_value);
}

bool operator == (Token const& a, std::string const& b)
{
  return b == a;
}

bool operator == (Token const& a, TokenKind const& b)
{
  return a.getKind() == b;
}

bool operator == (TokenKind const& a, Token const& b)
{
  return b == a;
}


struct Lexer::impl {
  static Token readToken(Lexer& lexer)
  {
    if (lexer.getPosition() == 0) {
      skipBom(lexer);
    }

    skipIgnored(lexer);

    if (!lexer.m_stream.good()) {
      if (lexer.m_stream.eof()) {
        return { TokenKind::ParseEnd, "" };
      } else {
        fail("Input stream error");
      }
    }

    if (check(lexer, s_sectionBegin)) {
      return readSectionBegin(lexer);
    } else if (checkIsKey(lexer)) {
      return readKey(lexer);
    } else if (check(lexer, s_keySeparator)) {
      return readKeySeparator(lexer);
    } else if (check(lexer, s_entrySeparator)) {
      return readEntrySeparator(lexer);
    } else if (check(lexer, s_sectionEnd)) {
      return readSectionEnd(lexer);
    } else if (check(lexer, s_valueBegin)) {
      return readValue(lexer);
    } else {
      return { TokenKind::ParseError, "Syntax error" };
    }
  }

  static bool checkIsKey(Lexer& lexer)
  {
    return isKeyBeginner(lexer.peekChar());
  }

  static void skipIgnored(Lexer& lexer)
  {
    char symbol = lexer.peekChar();
    while (isIgnored(symbol)) {
      lexer.getChar();
      symbol = lexer.peekChar();
    }
  }

  static void skipBom(Lexer& lexer)
  {
    constexpr char utf8bom[] = {
      static_cast<char>(0xEF),
      static_cast<char>(0xBB),
      static_cast<char>(0xBF)
    };

    if (check(lexer, utf8bom[0])) {
      lexer.getChar();
      expect(lexer, { utf8bom[1], utf8bom[2] }, "Wrong BOM");
    }
  }

  static bool isIgnored(char c)
  {
    return std::isspace(c, s_cLocale)
        || std::iscntrl(c, s_cLocale);
  }

  static bool isKeyBeginner(char c)
  {
    return std::isalpha(c, s_cLocale)
        || std::isdigit(c, s_cLocale)
        || (c == '_');
  }

  using CodePoint = int;
  static_assert(4 <= sizeof(CodePoint),
    "Target platform has too narrow 'int' type");

  // Make UTF-8 code point from escape sequence of 4 hex digits like '0xFFFF'.
  static CodePoint makeCodepoint(std::array<char, 4> const& escapeSequence)
  {
    using EscapeSequenceType = std::decay_t<decltype(escapeSequence)>;
    constexpr int escapeSequenceLength =
      std::tuple_size<EscapeSequenceType>::value;
    static_assert(4 == escapeSequenceLength,
      "Unexpected escape sequence length");
    constexpr int bitsPerDigit = 4;

    CodePoint codepoint = 0;
    for (int i = 0; i != escapeSequenceLength; ++i) {
      char const digit = escapeSequence[i];
      int const power = (escapeSequenceLength - 1 - i) * bitsPerDigit;

      if (('0' <= digit) && (digit <= '9')) {
        codepoint += ((digit - int('0')) << power);
      } else if (('a' <= digit) && (digit <= 'f')) {
        codepoint += ((digit - int('a')) << power);
      } else if (('A' <= digit) && (digit <= 'F')) {
        codepoint += ((digit - int('A')) << power);
      } else {
        fail("Unexpected symbol found in escape sequence");
      }
    }

    if (((0x0000 <= codepoint) && (codepoint <= 0xFFFF)) == false) {
      fail("Wrong escaped codepoint");
    }

    return codepoint;
  }

  static std::initializer_list<char> makeCodeunits(CodePoint codepoint)
  {
    if (codepoint < 0x80) {
      // 1-byte characters: 0xxxxxxx (ASCII)
      return {
        static_cast<char>(codepoint)
      };
    } else if (codepoint <= 0x7FF) {
      // 2-byte characters: 110xxxxx 10xxxxxx
      return {
        static_cast<char>(0xC0 | (codepoint >> 6)),
        static_cast<char>(0x80 | (codepoint & 0x3F))
      };
    } else if (codepoint <= 0xFFFF) {
      // 3-byte characters: 1110xxxx 10xxxxxx 10xxxxxx
      return {
        static_cast<char>(0xE0 | (codepoint >> 12)),
        static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)),
        static_cast<char>(0x80 | (codepoint & 0x3F))
      };
    } else {
      // 4-byte characters: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
      return {
        static_cast<char>(0xF0 | (codepoint >> 18)),
        static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)),
        static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)),
        static_cast<char>(0x80 | (codepoint & 0x3F))
      };
    }
  }

  static Token readKey(Lexer& lexer)
  {
    auto isKeyInternal = [&] (char c) {
      return std::isalpha(c, s_cLocale)
        || std::isdigit(c, s_cLocale)
        || (c == '_');
    };

    std::string buffer;
    char c = lexer.peekChar();
    while ((c != s_keySeparator) && !isIgnored(c)) {
      if (!isKeyInternal(c)) {
        fail("Unexpected symbol found in key");
      }
      buffer.append({ c });
      lexer.getChar();
      c = lexer.peekChar();
    }
    return { TokenKind::Key, std::move(buffer) };
  }

  static CodePoint readEscapedCodepoint(Lexer& lexer)
  {
    std::array<char, 4> escapeSequence = { 0 };
    for (char& elem : escapeSequence) {
      elem = lexer.getChar();
    }
    return makeCodepoint(escapeSequence);
  }

  static constexpr bool isHighSurrogate(CodePoint codepoint)
  {
    return (0xD800 <= codepoint) && (codepoint <= 0xDBFF);
  }

  static constexpr bool isLowSurrogate(CodePoint codepoint)
  {
    return (0xDC00 <= codepoint) && (codepoint <= 0xDFFF);
  }

  static constexpr int makeSurrogate(CodePoint codepoint1, CodePoint codepoint2)
  {
    return (codepoint1 << 10) + codepoint2 - 0x35FDC00;
  }

  static Token readValue(Lexer& lexer)
  {
    auto readEscaped = [&] () -> std::string {
      if (check(lexer, 'n')) {
        lexer.getChar();
        return { '\n' };
      } else if (check(lexer, 'r')) {
        lexer.getChar();
        return { '\r' };
      } else if (check(lexer, s_escape)) {
        lexer.getChar();
        return { s_escape };
      } else if (check(lexer, 'x')) {
        lexer.getChar();
        CodePoint codepoint = readEscapedCodepoint(lexer);
        if (isHighSurrogate(codepoint)) {
          CodePoint const highSurrogate = codepoint;

          expect(lexer, { s_escape, 'x' }, "Expected low surrogate in pair");
          CodePoint const lowSurrogate = readEscapedCodepoint(lexer);
          if (!isLowSurrogate(lowSurrogate)) {
            fail("Wrong low surrogate in pair");
          }
          codepoint = makeSurrogate(highSurrogate, lowSurrogate);
        }
        return makeCodeunits(codepoint);
      }

      fail("Unknown escape sequence");
    };

    auto readUnescaped = [&] () -> char {
      char const c = lexer.peekChar();
      if (std::iscntrl(c, s_cLocale)) {
        fail("Unexpected character");
      }
      lexer.getChar();
      // TODO: check for overlong UTF-8 sequences
      return c;
    };

    expect(lexer, s_valueBegin, "Expected value");

    std::string buffer;
    char c = lexer.peekChar();
    while (c != s_valueEnd) {
      if (check(lexer, s_escape)) {
        lexer.getChar();
        buffer.append(readEscaped());
      } else {
        buffer.append({ readUnescaped() });
      }
      c = lexer.peekChar();
    }

    return { TokenKind::Value, std::move(buffer) };
  }

  static Token readSectionBegin(Lexer& lexer)
  {
    expect(lexer, s_sectionBegin, "Expected section begin");
    return { TokenKind::KeyValueSeparator, { s_sectionBegin } };
  }

  static Token readSectionEnd(Lexer& lexer)
  {
    expect(lexer, s_sectionEnd, "Expected section end");
    return { TokenKind::KeyValueSeparator, { s_sectionEnd } };
  }

  static Token readKeySeparator(Lexer& lexer)
  {
    expect(lexer, s_keySeparator, "Expected key separator");
    return { TokenKind::KeyValueSeparator, { s_keySeparator } };
  }

  static Token readEntrySeparator(Lexer& lexer)
  {
    expect(lexer, s_entrySeparator, "Expected entry separator");
    return { TokenKind::EntrySeparator, { s_entrySeparator } };
  }


  static bool expect(Lexer& lexer, std::string const& expected,
    char const* failMessage)
  {
    if (!check(lexer, expected)) {
      fail(failMessage);
    }
    for (size_t i = 0; i != expected.size(); ++i) {
      lexer.getChar();
    }
  }

  static bool expect(Lexer& lexer, char expected,
    char const* failMessage)
  {
    if (!check(lexer, expected)) {
      fail(failMessage);
    }
    lexer.getChar();
  }

  static bool check(Lexer& lexer, std::string const& expected)
  {
    auto startPos = lexer.m_stream.tellg();

    auto iExpected = expected.begin();
    auto const iExpectedEnd = expected.end();
    char c = lexer.peekChar();
    while ((iExpected != iExpectedEnd) && (c == *iExpected)) {
        ++iExpected;
        lexer.getChar();
        c = lexer.peekChar();
    }
    bool const result = iExpected == iExpectedEnd;

    lexer.m_stream.seekg(startPos);

    return result;
  }

  static bool check(Lexer& lexer, char expected)
  {
    return lexer.peekChar() == expected;
  }

  static Token fail(std::string const& message)
  {
    throw Exception(message);
  }


  static std::locale const s_cLocale;
  static constexpr char s_keySeparator = ':';
  static constexpr char s_entrySeparator = ',';
  static constexpr char s_sectionBegin = '{';
  static constexpr char s_sectionEnd = '}';
  static constexpr char s_valueBegin = '"';
  static constexpr char s_valueEnd = '"';
  static constexpr char s_escape = '\\';
};

std::locale const Lexer::impl::s_cLocale = std::locale();

Lexer::Lexer(std::istream& is)
  : m_stream(is)
  , m_lastToken()
{}

Token const& Lexer::getCurrent()
{
  if (!m_lastToken || !isFinished()) {
    return getNext();
  }
  return m_lastToken;
}

Token const& Lexer::getNext()
{
  if (isFinished()) {
    return m_lastToken;
  }

  try {
    m_lastToken = impl::readToken(*this);
  } catch (Exception const& e) {
    m_lastToken = Token(TokenKind::ParseError,
      "Parse error at position " + std::to_string(m_stream.tellg()) + ": " +
      e.what());
  }
  return m_lastToken;
}

bool Lexer::isFinished() const
{
  return m_lastToken.getKind() == TokenKind::ParseEnd;
}

std::istream::pos_type Lexer::getPosition() const
{
  return m_stream.tellg();
}

char Lexer::getChar()
{
  if (m_stream.eof()) {
    throw Exception("Unexpected end of data");
  } else if (m_stream.bad()) {
    throw Exception("Internal stream error");
  }
  return m_stream.get();
}

char Lexer::peekChar() const
{
  return m_stream.peek();
}


struct Parser::impl {
  // Grammar is decomposed to:
  //
  // Start = Section
  // Section = SectionBegin Entries SectionEnd
  // SectionBegin = '{'
  // SectionEnd = '}'
  // Entries = ( Entry NextEntry )?
  // Entry = Key KeyValueSeparator Value
  // NextEntry = ( EntrySeparator Entry )?
  // EntrySeparator = ','
  // Key = key
  // KeyValueSeparator = ':'
  // Value = Section | TextValue
  // TextValue = value

  // Mapping from decomposed grammar to internal parser states
  enum class StateKind {
    Start = 0,
    Section,
    SectionBegin,
    SectionEnd,
    Entry,
    NextEntry,
    EntrySeparator,
    Entries,
    Key,
    Value,
    KeyValueSeparator,
    TextValue,

    _count // keep last
  };

  // Set of possible token parsing products
  enum class ProductKind {
    SectionBegin,
    SectionEnd,
    Entry,
    Key,
    Value
  };

  // Token parsing product
  struct Product {
    ProductKind m_kind;
    std::string m_value;
  };

  // Transition function result
  enum class ActionKind {
    Produce, // accept current token and produce some symbols
    Expect, // accept current token and expect some new tokens
    Fail // reject current token and return parse error
  };

  // TODO: implement using variant
  struct Action {
    ActionKind m_kind;

    struct Fail {
      ParsingErrorKind m_error;
      StateKind m_state;
      TokenKind m_receivedToken;
      std::istream::pos_type m_position;
    };
    struct Produce {
      std::vector<Product> m_producedSymbols;
    };
    struct Expect {
      std::vector<StateKind> m_expectedProductions;
    };

    Fail m_failure;
    Produce m_production;
    Expect m_expectation;

    static Action expect(std::vector<StateKind> states)
    {
      Action action;
      action.m_kind = ActionKind::Expect;
      action.m_expectation.m_expectedProductions = std::move(states);
      return action;
    }

    static Action produce(std::vector<Product> products)
    {
      Action action;
      action.m_kind = ActionKind::Produce;
      action.m_production.m_producedSymbols = std::move(products);
      return action;
    }

    static Action fail(StateKind state, TokenKind receivedToken,
      std::istream::pos_type position,
      ParsingErrorKind error = ParsingErrorKind::UnexpectedTokenReceived)
    {
      Action action;
      action.m_kind = ActionKind::Fail;
      action.m_failure.m_error = error;
      action.m_failure.m_state = state;
      action.m_failure.m_position = position;
      action.m_failure.m_receivedToken = receivedToken;
      return action;
    }
  };

  // Transition function
  static Action doTransition(StateKind const& state, Lexer& lexer)
  {
    auto check = [&] (TokenKind const& kind) {
      auto const& token = lexer.getCurrent();
      return token.getKind() == kind;
    };
    auto consume = [&] () -> Token const& {
      auto const& current = lexer.getCurrent();
      lexer.getNext();
      return current;
    };

    if (check(TokenKind::ParseError)) {
      return Action::fail(state,
        lexer.getCurrent().getKind(), lexer.getPosition());
    }

    switch (state) {
      case StateKind::Start:
        return Action::expect({ StateKind::Section });

      case StateKind::Section:
        return Action::expect({
          StateKind::SectionBegin,
          StateKind::Entries,
          StateKind::SectionEnd
        });

      case StateKind::SectionBegin:
        if (check(TokenKind::SectionBegin)) {
          consume();
          return Action::produce({
            { ProductKind::SectionBegin, "" }
          });
        }

      case StateKind::SectionEnd:
        if (check(TokenKind::SectionEnd)) {
          consume();
          return Action::produce({
            { ProductKind::SectionEnd, "" }
          });
        }

      case StateKind::Entries:
        if (check(TokenKind::Key)) {
          return Action::expect({ StateKind::Entry, StateKind::NextEntry });
        } else {
          return Action::expect({ /* none */ });
        }

      case StateKind::Entry:
        return Action::expect({
          StateKind::Key,
          StateKind::KeyValueSeparator,
          StateKind::Value
        });

      case StateKind::Key:
        if (check(TokenKind::Key)) {
          Token token = consume();
          return Action::produce({
            { ProductKind::Entry, "" },
            { ProductKind::Key, token.getText() }
          });
        }

      case StateKind::KeyValueSeparator:
        if (check(TokenKind::KeyValueSeparator)) {
          consume();
          return Action::produce({ /* none */ });
        }

      case StateKind::Value:
        if (check(TokenKind::Value)) {
          return Action::expect({ StateKind::TextValue });
        } else if (check(TokenKind::SectionBegin)) {
          return Action::expect({ StateKind::Section });
        }

      case StateKind::TextValue:
        if (check(TokenKind::Value)){
          auto const& token = consume();
          return Action::produce({
            { ProductKind::Value, token.getText() }
          });
        }

      case StateKind::NextEntry:
        if (check(TokenKind::EntrySeparator)) {
          return Action::expect({ StateKind::Entry });
        } else {
          return Action::expect({ /* none */ });
        }

      // no default for warning
    }

    return Action::fail(state,
      lexer.getCurrent().getKind(), lexer.getPosition());
  }

  // Factory function for errors
  static ParsingError makeParsingError(Action::Fail const& failure)
  {
    ParsingError error;
    error.m_kind = failure.m_error;
    error.m_position = failure.m_position;
    return error;
  }

  // Function to create category name in parsed tree
  static Key join(std::vector<Key> const& parts, std::string const& glue)
  {
    size_t const length = std::accumulate(parts.begin(), parts.end(), 0,
      [&] (auto const& sum, auto const& part) {
        return sum + part.size() + glue.size();
      });
    Key result;
    result.reserve(length);
    std::for_each(parts.begin(), parts.end(), [&] (auto const& part) {
      result.append(part).append(glue);
    });
    return result;
  }

  // Function to create resulting parsing tree
  static ParsedTree makeOutputTree(std::vector<Product> const& outputSequence)
  {
    // NOTE: assuming sequence satisfies grammar

    ParsedTree tree;

    std::vector<Key> sectionsStack;
    Key lastKey;

    for (auto const& product : outputSequence) {
      switch (product.m_kind) {
        case ProductKind::SectionBegin:
        {
          sectionsStack.push_back(lastKey);
          auto category = join(sectionsStack, { s_categorySeparator });
          tree.emplace(std::move(category), "");
          break;
        }

        case ProductKind::SectionEnd:
          sectionsStack.pop_back();
          break;

        case ProductKind::Entry:
          // none
          break;

        case ProductKind::Key:
          lastKey = product.m_value;
          break;

        case ProductKind::Value:
        {
          auto category = join(sectionsStack, { s_categorySeparator });
          tree.emplace(std::move(category), product.m_value);
          break;
        }

        // no default for warning
      }
    }

    return tree;
  }
};

Parser::Parser(std::istream& is)
  : m_lexer(is)
{}

Parser::ParsingResult Parser::parse() {
  // Parses the grammar as LL(1) using predictive LL(1) parser.

  using Action = impl::Action;
  using ActionKind = impl::ActionKind;
  using StateKind = impl::StateKind;
  using Product = impl::Product;

  ParsingResult result;

  std::stack<StateKind> states;
  states.push(StateKind::Start);

  std::vector<Product> outputSequence;

  auto fail = [&] (Action::Fail const& failure) {
    result.m_error = impl::makeParsingError(failure);
  };

  auto expect = [&] (Action::Expect const& expectation) {
    std::for_each(
      expectation.m_expectedProductions.rbegin(),
      expectation.m_expectedProductions.rend(),
      [&] (auto const& entry) {
        states.push(entry);
      });
  };

  auto accept = [&] (Action::Produce const& production) {
    for (auto const& entry : production.m_producedSymbols) {
      outputSequence.push_back(entry);
    }
  };

  auto doAction = [&] (Action const& action) -> bool {
    switch (action.m_kind) {
      case ActionKind::Expect:
        expect(action.m_expectation);
        return true;

      case ActionKind::Produce:
        accept(action.m_production);
        return true;

      case ActionKind::Fail:
        fail(action.m_failure);
        return false;

      // no default for warning
    }
    return false;
  };

  result.m_success = true;
  while (result.m_success && !states.empty()) {
    auto state = states.top();
    states.pop();
    auto action = impl::doTransition(state, m_lexer);
    result.m_success = doAction(action);
  }

  if (result.m_success) {
    result.m_tree = impl::makeOutputTree(outputSequence);
  }

  return result;
}

} // namespace parsing
