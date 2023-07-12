# How the Minifier Works

Broadly speaking the minifier, after loading the source code and splitting it into tokens,
makes multiple passes over the tokens, incrementally making it smaller before joining them
up and outputting the minified source to a file.

## Passes

1. Grouping - This pass (technically there is one per grouping) groups together subsets of the tokens into single tokens, for example collecting strings that were initially separated into one token, or collecting a whole deletion region into one token.

2. Stripping - This pass simply removes all code that does not need to be present in the final code, such as deletion regions, newline delimiters and comments.

3. Gathering Statistics - Thanks to the requirements of how C++ files are laid out, in this
single pass we can determine the structure of the program (example shown below) as well as detailed
statistics about how often different kinds of tokens dissapear in different contexts.

4. Intermediate Representation - Here all functions, function arguments, local variables
defined in functions and struct fields and methods are renamed into generic "{type}{num}" form,
decided by frequency, so in the below example, the method `setHash` would have the variable
`i` set to `var0`, `bb` set to `var1` and `sq` set to `var2`. This ensures that as many equivalent
variable names as possible are used between scopes, because when it comes to compression
**repetition is king**.

5. Renaming variables - This final pass takes the intermediate representation and simply renames
all the renamable tokens to smaller single-letter tokens.

## What to add next?

Most potential gains are probably to come from additional passes between pass 5 and 6, at the
moment different types of token `var`, `arg`, etc are almost completely separate, and so most often
use distinct identifiers from each other, when they could potentially share them.
It would be pretty funny to see `a.a.a.a()` or something similar in the minified source.

## Gathering Statistics - Example
Here is the result of the gathering statistics pass on the source code for the `BoardState` struct
when the feature was first added. The numbers shown are frequencies of the tokens.

```
Struct(name='BoardState',
       fields={   'boards': 29,
                  'castlingRights': 2,
                  'epSquare': 2,
                  'flags': 2,
                  'hash': 4},
       functions={   'attackedByOpponent': Function(args={'sq': 7},
                                                    variables={'bb': 3}),
                     'pieceOn': Function(args={'sq': 6}, variables={}),
                     'setHash': Function(args={},
                                         variables={'bb': 5, 'i': 7, 'sq': 2})},
       top_lvl_used=[   'getKnightMoves',
                        'getKingMoves',
                        'getOrthogonalMoves',
                        'getDiagonalMoves'])
```

## Intermediate Representation - Example
Here is the `searchRoot` function in IR. Note the `field0` token, which is there because
the variable shares a name with an existing field in one of the structs, so we can keep them
the same name for free with no worry about causing errors!

```
void func5(auto& arg1, auto& arg0, auto arg2, auto arg3) {
    auto field0 = 0;
    auto var2 = chrono::high_resolution_clock::now();
    for (auto var1 = 1; var1 < 64; var1++) {
        func4(arg1, arg0, 0, var1, -32000, 32000, var2 + chrono::milliseconds(arg2 / 40 + arg3 / 2));
        if (arg0.field1) field0 = arg0.field0;
    }
    cout << "bestmove " << func3(field0, arg1.field0.field2[0]) << endl;
}
```