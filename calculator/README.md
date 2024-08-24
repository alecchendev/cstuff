# calculator

A simple repl calculator. Inspired by the windows calculator that's 50,000 lines, I wondered
how simple I could make something that does very similar things. I would've actually made
this a graphical application, but I'm a C noob and don't want to deal with the build
system.

This is also a miniature version of some broader compiler techniques, to learn C better.

To do:
- [x] Parse tokens properly
    - [ ] Make token parsing more elegant/generic
    - [x] Handle more edge cases (quitX should be invalid)
- [x] Recursively handle parsing any token sequence
    - [x] Respect order of operations
    - [ ] Handle quit token in a parse leaf differently?
- [x] Recursively handle evaluating any expression
- [x] Write tests
    - [x] Basic tokenize tests
    - [x] Handle panics in tests
    - [ ] Display test result info better
    - [x] Unit checking test
        - [ ] For invalid units, don't print the error message in tests
    - [x] Parse tests (basic)
    - [x] Eval tests
    - [ ] Test memory abuse
    - [ ] Test for undefined behavior (spam tests over and over - actual fuzzing is probably just overkill)
    - [ ] Overall more comprehensive tests
- [ ] Make better error messages for parsing
    - [ ] Don't collapse tree upon an invalid leaf
- [ ] Handle other operations (pow/sqrt, parentheses, log)
- [x] Tokenize scientific notation
- [x] Allow running a single command instead of a repl
- [x] Handle arrow keys (history, moving left/right)
    - [x] Refactor to process key input
    - [x] Hold state + redraw line for backspace
    - [x] Implement left right
    - [x] Implement history
- [x] Allow infinite size input and handle memory failures
- [x] Optimize memory a little bit
- [ ] Handle other text editing (e.g. option/command + backspace)
- [x] Add debug logs configurable via a flag
    - [ ] Figure out a good way to handle memory here for nontrivial display functions
    - [ ] Figure out display_expr, printing multiple times without new line - or just restructure function to return string
- [ ] Address every TODO in the code
- [x] Handle units and conversions
    - [x] Basic rejection of incompatible units
    - [x] Have a set of builtin units with conversion rates out of the box
- [ ] Composite units
    - [x] Check/build composite units
    - [x] Handle negation
    - [ ] Tokenize + parse composite units
        - [x] Degrees of individual units
        - [ ] Multiple units
    - [ ] Handle composite conversions
- [ ] Handle performing operations on the last result (expression starts with operation)
- [ ] Let user define stuff
    - [ ] Handle basic variables
    - [ ] Handle basic scripts
    - [ ] Handle custom units
    - [ ] Handle custom conversions
- [ ] Display current memory in repl (variables, custom unit definitions)
- [ ] Compile to webassembly and make a basic webapp (no backend, obviously)
    - [ ] Add autocomplete for builtin units when typing
    - [ ] Syntax highlightning? maybe need to tokenize/parse along the way..?

