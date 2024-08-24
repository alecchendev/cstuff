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
    - [x] Parse tests (basic)
    - [x] Eval tests
    - [ ] Test memory abuse
    - [ ] Test for undefined behavior (spam tests over and over - actual fuzzing is probably just overkill)
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
- [ ] Address every TODO in the code
- [x] Handle units and conversions
    - [x] Basic rejection of incompatible units
    - [ ] Have a set of builtin units with conversion rates out of the box
    - [ ] Handle custom units?
    - [ ] Tokenize, parse, check composite units
- [ ] Handle basic variables
- [ ] Handle basic scripts
- [ ] Compile to webassembly and make a basic webapp (no backend, obviously)

