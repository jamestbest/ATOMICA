Starting the parser!

Create a design for the node
 - small - data oriented
 - probably linked to a token

hand written recursive descent parser

shunting yard for expressions - over pratt parsing

Making array - put at end of video

top level statements
 - variable declaration
   - parsing types 
   - including functions
   - data statements
 - function calls
 - assignment
 - while
 - capture | | 
 - question op ?

flags in the tag e.g. is end child
{  
   x: i4  
   y: i4  
   z: i4
}

CSNode
XDecl
YDecl
ZDecl - is end child

y's tag would include the end child bit set
if empty then a dummy node would be required

Most nodes are 

{
   token: Token         (4 bytes)
   tag: Tag.7
   is_end_child: n1.1   
   data: n4.24          (3 bytes)
}

OR soa way

{
   token: []Token
   tag: []Tag
   lhs: []Index
   rhs: []Index
}

CS {
   token
   tag
   lhs: 10
   rhs: 5
}

extra
[ ... 10 ... 14 ]
[ ... 60, 72, 80, 90, 100, ...]

a Node is an index into this soa

fixing problems
fixing performance problems

clang -Xclang -ast-dump
