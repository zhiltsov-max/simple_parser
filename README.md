# Simple parser

Implementation of lexer, parser and transformer for simple EBNF-grammar.
Parser implemented with predictive lookahead LL(1) algorithm.
Supports input streams in ANSI and UTF-8 encodings.

## Grammar

```
escape = '\'
escaped = escape ( 'r' | 'n' | '\' | 'x' [0-9]{4} )
unescaped = [^\\x00-\x20]
char = escaped | unescaped
value = '"' char* '"'
key = [a-zA-Z0-9_]+
entry = key ':' ( value | section )
entries = entry ( ',' entry )?
section = '{' ( entries )? '}'

START = (UTF-8 BOM)? section
```

Example:
```
{
  x: "5",
  55y : {
    _z7: "sdf\n3443\r"
  },
  qq :"hello",
  utf8_value: "привет"
}
```

## Installation

### Build

``` bash
git clone <this repo>
mkdir build && cd build
cmake ..
cmake --build .
```

### Usage

The code might be integrated to CMake projects with the following steps:

``` cmake
find_package(Parser REQUIRED)
...
target_link_libraries(your_target
  PRIVATE # or PUBLIC, or INTERFACE
    Parser::Parser
  )
```

All the definitions and dependencies will be added automatically.


### Running unit tests

``` bash
cmake . -DBUILD_TESTING=ON
cmake --build .
cmake --build . --target unit_tests
```