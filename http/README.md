# cstuff

Idk, for now just a simple HTTP server + client in C for fun.

To do:
- [ ] Make different versions of the server
    - [x] Single threaded, blocking
    - [ ] Multithreaded (just have a fixed size thread pool), blocking
    - [ ] Async with a basic scheduler or smth...????
- [ ] Actually implement parsing HTTP stuff
    - [ ] Server
    - [ ] Client
    - Main stuff:
        - Method
        - path
        - params - path, query, data
        - Headers
- [ ] Actually handle errors instead of just exiting
