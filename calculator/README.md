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
- [x] Recursively handle evaluating any expression
- [x] Write tests
    - [x] Basic tokenize tests
    - [x] Handle panics in tests
    - [ ] Display test result info better
    - [ ] Parse tests
    - [x] Eval tests
    - [ ] Test memory abuse
- [ ] Handle other operations (pow/sqrt, parentheses, log)
- [x] Tokenize scientific notation
- [x] Allow running a single command instead of a repl
- [x] Handle arrow keys (history, moving left/right)
    - [x] Refactor to process key input
    - [x] Hold state + redraw line for backspace
    - [x] Implement left right
    - [x] Implement history
- [ ] Handle other text editing (e.g. option/command + backspace)
- [ ] Add debug logs configurable via a flag, that can write to a file
- [ ] Address every TODO in the code
- [ ] Handle units and conversions

