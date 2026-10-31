Designing a lexer that is 'data oriented'

Streaming vs one-shot


Making tokens small
 - one n4
 - split tag & offset

Deferring literal computation

Metaprogramming
 - appending to the buffer

The langauge is state-less on the line level for tokens
 - strings
 - comments

allows for parallelism, although how useful when lexing
is cheap
