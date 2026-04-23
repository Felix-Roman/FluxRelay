FluxRelay a Multi-Client Chat System Using Event-Driven I/O

Overview and Goals:
-FluxRelay is a multi-client chat system implemented using non-blocking sockets and an event-driven server architecture. The system allows multiple clients to connect to a central server, send messages, and receive broadcasts in real time.
The primary goal of this project is to demonstrate how scalable network systems can efficiently manage multiple concurrent connections without relying on multi-threading. 

Themes Used:
-Event-Based Concurrency:
  Instead of spawning threads for each client, the server uses a single event loop using select() to monitor multiple file descriptors. This allows efficient        handling of multiple clients simultaneously.

-Network Programming:
  The system uses TCP sockets for communication between server and clients. It demonstrates socket creation, binding, listening, accepting connections, and data     transmission.

-System-Level I/O:
  FluxRelay directly interacts with low-level system calls such as recv(), send(), and select(), providing hands-on experience with operating system                 interfaces.

-Control Flow Management:
   The event loop requires careful control flow to handle new connections, incoming messages, and disconnections without blocking execution.

Design Decisions and Trade-Offs:

-Event Loop vs Multithreading:
-Used: Event driven loop using select()
-Trade off: Simpler and avoids thread overhead, but less scalable than epoll for very large systems.

-Non-Blocking Sockets:
-Used: Non-blocking sockets for responsiveness
-Trade off: Requires careful handling of return values since recv() may return -1

-Fixed Buffer Size:
-Used: Static buffer 1024 bytes
-Trade off: Simpler memory management, but limits message size and risks truncation

-Simple Broadcast Model:
-Used: All messages sent to all clients
-Trade off:Easy to implement, but lacks features like private messaging or rooms


Challenges Encountered and Lessons Learned:

-Challenges:
-Debugging select() behavior and ensuring proper file descriptor tracking
-Handling non-blocking socket behavior with recv() returning -1 vs 0 sometimes
-Fixing subtle C syntax bugs like accidental semicolons or breaks in logic
-Managing client state correctly like active, inactive, and tracking them

-Lessons Learned:
-Small syntax mistakes in C can cause major logic failures
-Proper initialization like client arrays is critical
-Event driven programming requires careful state management
-Debugging network programs requires isolating layers client vs server


Features:
-Multi-client support
-Real-time message broadcasting
-Automatic user session management
-Connection and disconnection handling
-Event-driven server using select()


Compilation and Usage:

-Compile:
-gcc server.c -o server
-gcc client.c -o client

-Run:
./server in the first terminal
./client then do this command in another terminal tab to test the clients
-Run multiple clients in separate terminals to test communication.

Limitations:
-No private messaging
-No persistent storage or chat history
-Fixed number of clients
-No GUI just a text-based interface only

Future Improvements:
-Add command for custom usernames
-Implement private messaging
-Upgrade to something like epoll for scalability
-Add a loggin system
-Improve input/ouput formatting
---******

