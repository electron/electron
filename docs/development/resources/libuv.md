# Libuv Resources

Understanding Nodejs better is helpful for working with electron. Node's event loop is implemented using the libuv library, created by the node team. It replaces the libeio and libenv, the original libraries used to implement the event loop. 

Roughly, the event loop is callbacks -> sleep -> callbacks -> sleep. Whenever there isn't work to do, the event loop is sleeping.

As a programmer, you want to be able to queue I/O operations, and asynchronously get a result back. The highest level view of libuv is roughly:

requests are similar to promises. They represent a result in the future.

handles are roughly event emitters

Being somewhat familiar with the libuv constructs make the references in source code much less ominous. You can view: https://www.youtube.com/watch?v=nGn60vDSxQ4 if you want to understand better.