some clean-up from last video
-X- add paren to if, do-while, while
-X- is_valid_expr start cannot be used in exclamation and question disambiguation
    if capture is allowed after

-X- captures- 005
-X- Code blocks ${}

Thinking about metaprogramming
Two sections
- #insert with code blocks
- #run 
  - having a single pass before that gathers the token index of each top level statement
  - dependencies of run expressions

entry main :: () : !void  {
    
}

funcA :: (): i4 {
    ret funcB() * funcB()
}

funcB :: (): i4 { ret 12 }

#run funcA()
- funcA
- funcB
there are no inner dependencies, only that we need to parse both

funcA :: (): i4 {
    a : i4 : #run funcB()
    ret a * a
}

funcB :: (): i4 {
    ret 12
}

#run funcA()

here there are dependencies in the runs
-> funcA -> funcB
we have to parse funcB first and then run, then do funcA

circular dependencies are errors
-> funcA -> funcB -> funcA ->


first pass ignore all directives, collect top level

at token identifier or keyword entry enter eat mode
 - eat all tokens up to either delimiter or LBRACE
at token lbrace eat all tokens up to rbrace (balanced)

special(?) cases for #insert and #run, and probably other directives about how to consume what follows

assume this works we now have a location for each top level statement and it's tokens
aim is to split this work up and do parsing over multiple threads (as well as other parts, i.e. SA, Code gen)
but what if they use #insert, 
    their token stream source needs to change, this source will be lexed, parsed, and evaluated
entry main :: (): !void {
    #insert ${ x: i4= 0 }
    #insert func()
}

func :: (): Code {
    t: Type= i4
    ret ${ y: \(t)= 0 }
}

${} is a token source, similar to how a string would be
things within #insert should be evaluated at compile time

could there be a special token type TT_TOKEN_SOURCE
that points to a source object? The usefulness of this depends on
how big the ${}'s are, and how much inserting and prepending is being done

because insert can add into the token stream it may be better to split off the 
token streams if they have an #include
These would also be represented as a token source, but without escapes
TLS's without #inserts can just point into the normal token stream as they won't add tokens

each just has a stack of token sources that at the end pops off and ${} adds to
#insert ${ x: #insert ${ i4 } } = 0


#run
run is just like insert but it doesn't add to the token stream
Supporting this means mapping dependencies of compilation
within a TLS we can assume that unknown elements at point X don't exist, i.e. that everything is forward declared
however they could depend on other TLSs or other imports

main :: (): !void {
    funcA :: (): i4 {
        ret 1
    }

    funcB :: (): i4 {
        ret 1
    } 

    #run print(funcA() + funcB())
}

we pause at the run and check dependencies
 - funcA: local, parsed
 - funcB: local, parsed
 - print: foreign, ?

I assume we create jobs that threads can pick up, and dependency's put them in blocked.  
so, there would be a job for semantic analysis for funcA and funcB and then Bytecode generation
once these are done main can continue parsing.

but, what if funcA uses some global value that hasn't been inserted yet  
need some way to say depends on unknown global value, and when global scope is updated with new inserts
 then we can check again

otherwise we'd have
 - parse main
 - parse funcA
 - parse funcB
 - PAUSE main
 - SA funcA
 - SA funcB
 - gen funcA
 - gen funcB
 - CONTINUE main
 - execute expression in VM
somewhere in here, perhaps before print would have had parse, SA, and gen





repeat :: (count: int, block: Code): Code {
    ret ${ for count { \block } }
}

main :: (): !void {
    #insert repeat(3, ${ print("Hello world\n") })
    for 3 { print("Hello world\n") }
}

repeat :: (count: int, block: Code): Code {
    res: Code= ${}
    for count {
        res+= ${ \(block);\n }
    }
    ret res
}

main :: (): !void {
    #insert repeat(3, ${ print("Hello world\n") })
    print("Hello world\n");
    print("Hello world\n");
    print("Hello world\n");
}

unroll :: (total_iters: id, unroll_count: int, block: Code) #expand { 
    #insert ${ for (\(total_iters) / \(unroll_count)) { }
    for unroll_count with UNROLL_IDX {
        #insert block
    }
    #insert ${ \} } 
}

unroll :: (total_iters: id, unroll_count: int, block: Code): Code {
    res: Code= ${ for (\(total_iters) / \(unroll_count)) { }
    for unroll_count with UNROLL_IDX {
        res += ${\t \(block) }
    }
    res += ${ \} }
    
    ret res
}

unroll :: (total_iters: id, unroll_count: int, block: Code): #inject Code

main :: (): !void {
    count := get_value()
    assert(count % 4 == 0)

    unroll($count, 4, ${ print("process(%)\n", i + `UNROLL_IDX) })
    #insert unroll($count, 4, ${ print("process(%)\n", i + `UNROLL_IDX) })
    unroll($count, 4, ${ print("process(%)\n", i + `UNROLL_IDX) })
    for count / 4 {
        print("process(%)\n", i + 0)
        print("process(%)\n", i + 1)
        print("process(%)\n", i + 2)
        print("process(%)\n", i + 3)
    }
}

== Semantic analysis
semantic analysis errors might not be errors
-- scope checking
The item may be a global item that has not yet been #inserted
 potentially even from another file
Don't have to worry about inner scopes because they are forward declared 
-- type checking
This is after scope checking, so we have the type
the only thing to consider is that types are values

== The plan

- (misc) ignore delims if \ precedes

- Do first pass over file
    - collect TLS
    - create token sources for each
    - move token sources for those that use #insert, so they can grow
-X- Create job struct
-X- Create multithreaded queue for jobs
-X- Create worker threads

-X- parse ${ }
- parse directives
-X- maybe: think about capture again

-X- think about the next stage
  - either some kind of untyped IR like in ZIG -- this could be flat, inline
  - or modify the ast nodes to start having connections
- then the byte code

- when we find imports, we have to check if it's been imported, and then lex etc.

== misc
Strings could have a bit in the size to determine if they are static or dynamic  
probably 0 for dynamic as that is most likely what users would be creating functions for  
then functions like free(s: string') could check this bit 
