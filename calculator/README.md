# calculator

A simple repl calculator. Inspired by the windows calculator that's 50,000 lines, I wondered
how simple I could make something that does very similar things. I would've actually made
this a graphical application, but I'm a C noob and don't want to deal with the build
system.

This is also a miniature version of some broader compiler techniques, to learn C better.

To do:
- [x] Parse tokens properly
    - [x] Handle more edge cases (quitX should be invalid)
- [x] Recursively handle parsing any token sequence
    - [x] Respect order of operations
    - [x] Handle quit token in a parse leaf differently?
- [x] Recursively handle evaluating any expression
- [x] Write tests
    - [x] Basic tokenize tests
    - [x] Handle panics in tests
    - [ ] Display test result info better
    - [x] Unit checking test
        - [x] For invalid units, don't print the error message in tests
    - [x] Parse tests (basic)
    - [x] Eval tests
    - [ ] Test memory abuse
    - [x] Test for undefined behavior (just run `while [test command]; end`)
    - [ ] Overall more comprehensive tests
- [x] Make better error messages for parsing
    - [x] Don't collapse tree upon an invalid leaf
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
- [ ] Handle other text editing in repl (e.g. option/command + backspace)
- [x] Add debug logs configurable via a flag
    - [ ] Figure out a good way to handle memory here for nontrivial display functions
    - [x] Figure out display_expr, printing multiple times without new line - or just restructure function to return string
- [ ] Address every TODO in the code
- [x] Handle units and conversions
    - [x] Basic rejection of incompatible units
    - [x] Have a set of builtin units with conversion rates out of the box
- [x] Composite units
    - [x] Check/build composite units
    - [x] Handle negation
    - [x] Tokenize + parse composite units
        - [x] Degrees of individual units
        - [x] Multiple units
        - [x] Handle dividing purely in units (i.e. "50 m / s")
        - [x] Reject units of same type (i.e. kg lb in one)
    - [x] Handle composite conversions
- [x] Unit aliases? I.e. force or newton -> kg m s^-2
- [ ] Let user define stuff
    - [x] Handle basic variables
    - [ ] Handle basic scripts
    - [ ] Handle custom units
    - [ ] Handle custom conversions
- [ ] Command to current memory in repl (variables, custom unit definitions)
- [ ] Command to show examples/how to use this thing
- [ ] Compile to webassembly and make a basic webapp (no backend, obviously)
    - [ ] Remove any printf's in release
    - [ ] Button to send a bug (send last input line)
    - [ ] Button to send request for builtin units/conversions/operations
