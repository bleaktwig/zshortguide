# ZMQ
> first, run `man zmq` to be formally introduced to our friendly ZMQ.

---
# Preliminary Notes
* Unlike common applications, a "socket" does not represents a connection to another node, but a socket can be used to communicate with many nodes. The same applies for a "thread".
* As far as I've seen, all ZMQ functions have a verbose and well-written `man` page. Try that before asking the internet, you might be surprised by the results.
* Compilation is pretty straightforward: `gcc -Wall -g FILENAME.c -lzmq -o FILENAME`. `-lzmq` links the ZMQ bindings, while `-Wall` and `-g` are what you should be using anyway.
* All the examples in the `examples` directory come from [**zguide**](https://github.com/booksbyus/zguide) and thus are licensed under the Creative Commons Attribution-Non-Commercial-Share Alike 3.0 License.

---
# Context
Applications always start by creating a context, which is how a ZMQ instance is created, and they end by destroying this context. In C:

* `zmq_ctx_new()`: Start a new context.
* `zmq_ctx_destroy()`: Destroy the current context.

If you use the `fork()` system call, each process needs its own context. If you do `zmq_ctx_new()` in the main process before running `fork()`, each child process gets its own context.

---
# The Socket API
> `man zmq_socket` is your friend. Remember to seek its help in moments of need.

ZMQ sockets live in four parts:

* The karmic circle of creation and destruction, or `zmq_socket()` and `zmq_close()`.
* Setting and checking configuration, or `zmq_setsockopt()` and `zmq_getsockopt()`.
* Plugging them into a network topology by creating ZMQ connections with `zmq_bind()` and `zmq_connect()`.
* Writing and receiving messages on them with `zmq_msg_send()` and `zmq_msg_recv()`.

Sockets are always void pointers and messages are structures. Therefore, **all your sockets are belong to ZMQ**, but messages are things that you own in our code. Sockets can only be created, destroyed and configured.

### Plugging Sockets into the Topology
* A server does `zmq_bind()` on a well-known address, becoming an endpoint bound to a socket. When a socket is bound to an endpoint, **it automatically starts accepting connections**.
* A client does `zmq_connect()` with an unknown or arbitrary network address, connecting through the socket to an endpoint.

Unlike a traditional TCP connection, if a network connection is broken, ZMQ will automatically reconnect. Also, the application code doesn't need to deal with the connections directly, it only deals with the socket.

It's worth noting that a client can connect to a socket before it is bound to a server. At some stage, the server will bind this socket and will receive all the queued messages, tho if they pile too much they might be discarded by ZMQ.

A server can bind to many endpoints, even using a single socket. With most transports you cannot bind the same endpoint twice, but `ipc` allows a process to bind to an already used by another one. This is meant to allow a process to recover after a crash.

### Using Sockets to Carry Data
* ZMQ sockets carry length-specified binary data, or message, rather than a stream of bytes as TCP does.
* Messages arrive in local input queues and are sent from local output queues.
* ZMQ sockets have a 1-to-N routing behaviour built-in.

### Unicast Transports
ZMQ provides three kinds of unicast transports:

* `tcp`: Elastic, portable and usually fast enough. It doesn't require a server or client to be bound or connected to the socket in order to work. It's what's best to use in most cases.
* `ipc`: Also disconnected, used for inter-process communication. It doesn't yet work on Windows, and apparently never will be ([issue pending on github since 2011](https://github.com/zeromq/libzmq/issues/153 "https://github.com/zeromq/libzmq/issues/153")).
* `inproc`: Inter-thread transport. While it is much faster than `tcp` or `ipc`, it is connected and thus requires the server to issue a bind request before any client issues a connect.

### ZMQ is not a Neutral Carrier
ZMQ imposes a framing on the transport protocol it uses, which is not compatible with existing protocols.

Since ZMQ v3.3, ZMQ has a socket option called `ZMQ_ROUTER_RAW` that lets us read and write data without the ZMQ framing. With this, we can, for example, connect to an arbitrary HTTP server from a ZMQ application.

### I/O Threads
When you create a new context, it starts with one I/O thread (for all sockets), which is sufficient for all but the most extreme applications (One I/O thread should be allowed per GB of data in or out per second). To raise the number of I/O threads, we use the `zmq_ctx_set()` call *before* creating any sockets:

    int io-threads = 4;
    void *context = zmq_ctx_new ();
    zmq_ctx_set (context, ZMQ_IO_THREADS, io_threads);
    assert (zmq_ctx_get (context, ZMQ_IO_THREADS) == io_threads);
**

If we are using ZMQ for inter-thread communication only, we can set the I/O threads to zero. This isn't a significant optimization though, more of a curiosity.

---
# Messaging Patterns
> `man zmq_socket` also lists the messaging patterns. It truly is your friend.

ZMQ routes and queues messages according to precise recipes called *patterns*, which encapsulate "good" ways to distribute data and work. ZMQ patterns are hard-coded, prohibiting by design user-definable patterns.

ZMQ patterns are implemented by pairs of sockets with matching types:
* *request-reply*: connects a set of clients to a set of services. This is a remote procedure call and task distribution pattern.
* *publish-subscribe*: connects a set of publishers to a set of subscribers. This is a data distribution pattern.
* *pipeline*: connects nodes in a fan-out/fan-in pattern that can have multiple steps and loops. This is a parallel task distribution and collection pattern.

There's another "pseudo-pattern": *exclusive pair*, which connects two sockets exclusively, and should only be used to connect two threads in a process.

For a connect-bind pair, these are the valid socket combinations:
* PUB and SUB
* REQ and REP
* REQ and ROUTER
* DEALER and REP
* DEALER and ROUTER
* DEALER and DEALER
* ROUTER and ROUTER
* PUSH and PULL
* PAIR and PAIR
* XPUB and XSUB (raw version of PUB and SUB)

Combinations not listed here will produce undocumented and unreliable results, commonly returning errors.

### High-Level Messaging Patterns
On top of the four mentioned patterns, *high-level patterns* exist for that extra :fire:spice:fire:. These are not part of the core library and do not come with the ZMQ package, but rather exist in their own space of the ZMQ community.

### Working with Messages
ZMQ messages are blobs of any size from 0 upwards, as long as they fit in memory. ZMQ doesn't touch serialization, so you need to do it yourself (no excuses - use profobufs, msgpack, JSON, etc).

A ZMQ message is a `zmq_msg_t` structure:

* `zmq_msg_t` objects are created and passed around, not blocks of data.
* To read a message, `zmq_msg_init()` is used to create an empty message, which is then passed to `zmq_msg_recv()`.
* to write a message, `zmq_msg_init_size()` is used to create a message and allocate a block of data of some size, which is then filled using `memcpy()` and sent with `zmq_msg_send()`.
* A message is released with `zmq_msg_close()`, which drops the reference. ZMQ will eventually destroy it.
* `zmq_msg_data()` is used to access the message content, and it's size is checked with `zmq_msg_size()`.
* `zmq_msg_move()`, `zmq_msg_copy()` and `zmq_msg_init_data()` are strictly forbidden unless we read the manual pages and know precisely **why** we need them. `zmq_msg_init_data()` is **extremely** forbidden, unless you **need** to worry about shaving microseconds.

After sending a message, ZMQ will clear it and its data cannot be accessed. If we want to send a message more than once, we need to create a second one, initialize it with `zmq_msg_init()` and use `zmq_msg_copy()` to create a copy of the first one (this does not copy the data, only the reference). This new message can then be sent, and ZMQ will clear the contents only after all copies have been sent.

Messages are composed of frames or "message parts", which are a length-specified block of data. Messages are then called *multiparts*, and it's worth remembering that in the low-level API each part is sent and received separately but the higher-end API provide wrappers to send entire multipart messages.

Some other stuffs are worth knowing about messages:

* You may send zero-length messages, e.g: for signaling.
* ZMQ guaranties that either all parts of a message will be delivered or none of them will.
* A message is not sent right away, so it must fit in memory, single or multipart.
* If you want to send a file, it's useful to break it up into pieces first, sending each piece as a separate single-part message.
* When finished with a message, you **always** need to call `zmq_msg_close()`.

**The quick and dirty way to work with ZMQ messages**: Use the `s_recv(socket)` and `s_send(socket)` methods from `exercises/zhelper.h`. They're lifesavers.

### Multipart Messages:
* Each part of a multipart message is sent using `zmq_msg_send(socket, &message, ZMQ_SNDMORE)`, while the last message is sent using `zmq_msg_send(socket, &message, 0)`
* Multipart messages are received in a loop, using `zmq_getsockopt(socket, ZMQ_RCVMORE, &more, &moresize)` to know if there is more content to receive or not. When `more = 0` you have received the whole message.
* You will receive all parts of a message, or none at all.
* The only way to close a partially sent message is to close the socket (of course, this isn't advised).

### The Dynamic Discovery Problem
It's hard to keep a large network running by itself, unless we hard-code it (which introduces fragility anyway). The common way to fix this in ZMQ is to use an **intermediary**, that is, a static point in the network to which all other nodes connect. The different messaging patterns require different intermediaries:

* For the PUB/SUB pattern, we use the XPUB/XSUB intermediary. PUB(s) connect to a proxy via its XSUB socket, and this proxy send the messages to the SUB(s) using its XPUB socket.
* For the REQ/REP, we can add flexibility to the network by using a broker with the DEALER/ROUTER sockets. REQ(s) connect to the broker through the ROUTER socket, which sends the messages to the REP(s) through the DEALER socket. DEALER and ROUTER are non-blocking.

It turns out that the common broker with a ROUTER and DEALER sockets is so useful that it's included in ZMQ as a stand-alone function: `zmq_proxy(frontend, backend, capture)`, where `frontend` is the socket facing clients, `backend` is the socket facing services and `capture` is an optional socket to capture the data sent. In practice, you should usually stick to ROUTER/DEALER, XSUB/XPUB and PULL/PUSH for the `frontend` and `backend` sockets. This is implemented in `exercises/msgqueue.c` if you wanna take a gander.

### Transport Bridging
A bridge is a small application that speaks one protocol at one socket and converts to/from a second protocol at another. An interpreter, if you like. A bridge can be built, for example, to communicate PUBs based on ZMQ to SUBs based on another type of network, while the bridge connects to an XSUB and an XPUB sockets.

### Handling Errors and ETERM
The ZMQ standard is to be as vulnerable as possible to internal errors and as robust as possible against external attacks and errors. In C/C++, assertions stop the application immediately with an error. If ZMQ detects an external fault, it returns an error to the calling code.

**Real code should do error handling on every single ZMQ call.** Here are the simple rules:
* Methods that create objects return NULL if they fail.
* Methods that process data may return the number of bytes processed, or -1 on an error or failure.
* Other methods return 0 on success and -1 on an error or failure.
* The error code is provided in `errno` or in `zmq_errno()`.
* Descriptive error text for logging is provided by `zmq_strerror()`.

An example on handling ZMQ errors:

    void *context = zmq_ctx_new ();
    assert (context);
    void *socket = zmq_socket (context, ZMQ_REP);
    assert (socket);
    int rc = zmq_bind (socket, "tcp://*:5555");
    if (rc != 0) {
        printf ("#: bind failed: %s\n", strerror (errno));
        return -1;
    }

There are two main exceptional conditions you may want to handle as nonfatal:
* When a thread calls `zmq_msg_recv()` with the `ZMQ_DONTWAIT` option and there is no waiting data, ZMQ will return -1 and set `errno` to `EAGAIN`.
* When a thread calls `zmq_ctx_destroy()` and other threads are doing blocking work, the `zmq_ctx_destroy()` call closes the context and all blocking calls exit with -1 and `errno` is set to `ETERM`.

**IMPORTANT NOTE**: In C/C++, asserts can be removed entirely in optimized code, so don't make the mistake of wrapping the whole ZMQ call in an `assert()` (basically what I did in `hs_client.c` and `hs_server.c`). It looks neat, then the optimizer removes all the asserts and the calls you want to make, and your application breaks in impressive ways.

### Shutting Down a Process Cleanly
In many applications, the only way to actually stop a process is by hitting `Ctrl-C`. In practice, we want a way to shut down a process cleanly. To do this, you need to find the node in the network that really knows when all processing is done, and send a `kill` message to all the nodes that don't shut down by themselves.

### Handling Interrupt Signals
Realistic applications need to shut down cleanly when interrupted with `Ctrl-C` or a signal such as `SIGTERM`. You can do this by catching these signals and exiting gracefully, instead of leaving the lights on when leaving the room. `examples/interrupt.c` shows an elegant and simple way to this.

### Detecting Memory Leaks
Don't be a doofus. Use [**Valgrind**](http://valgrind.org/) when debugging. Build with `-g` and `-DDEBUG` for additional debugging information, and run your program with `valgrind --tool=memcheck --leak-check=full --suppressions=valgrind.supp <YOURPROGRAM>`. `valgrind.supp` is located in the `examples` directory.

---
# Additional Information
### The Standard ZMQ Answer
The standard ZMQ answer to problems is to create a new socket flow for each type of problem you need to solve. Attempting to reuse already created sockets for new purposes leads to bloated and unwieldy code.

### Cleaning up after the Job
Just as you want to avoid a memory leak or a messy murder scene, it is **very** important to clean up after finishing the job; mainly for security concerns. The ZMQ objects you need to worry about are messages, sockets and contexts:

* Always close a message once you're done with it with `zmq_msg_close()`.
* If you are opening a ton of sockets, that's probably a sign that you need to redesign your application, and correctly setting the `LINGER` value should be a priority.
* Before a program exits, all sockets should be closed and `zmq_ctx_destroy()` should be called.

Exiting multithreaded applications is way more complicated and they should only be used when strictly necessary, since ZMQ handles stuff concurrently by itself (the ZMQ sockets act like little concurrent servers).
