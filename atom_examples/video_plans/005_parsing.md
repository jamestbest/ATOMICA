cleanup
 - removing newlines as delimiters
 - keeping them in the array but skipping in current, consume, etc
   - don't skip after consume, only at start - to allow for catching them
 - delimiters should be compressed
 - nls are used when printing or to get the line/col
   - but otherwise ignored
     - OR maybe required in some places/disallowed
       - e.g. 
         if x
           y
or not...
? is ambiguous without newline
 - e.g. 
   - x := func()?
   - y++

todo: make parens req for for, while, ifs

detecting unary operators
- probably by using expected?
- doing this before update expected
- The token can be the operator & node be the type
    - i.e. minus, binary vs unary
- prefix - expected
- postfix - next is valid start expr

printing of nodes -- probably moved to the end
including printing token positions when unknown 
 - can probably cache the line & line_index of last print
   - would work if they are printed sequentially
   - if index >= line_index then col= index - line_index, line= line
   - else redo the whole thing :(
 - doesn't need to be efficient, not normal compiler behaviour

problems
 - | | is ambiguous in every way for capturing x|y|z is either capture or bitwise
   - x || z -- empty capture?
   - fine if limited to x |a| {z}, I think? and no empty ones
