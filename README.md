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
