/***************
* Samir Yuja sjy14b
* Due 11/29/2016
* PROJECT 2
* COP 5570
***************/

README
To run : "make all"
Server: ./server.x user_info config_file
Client: ./client.x cconfig

Note: "user_info" and "config_file" must be provided, but
Server creates "cconfig" for client. (Please start server first!!)
Server writes a new "user_info" when it closes.

I/O Multiplexing:
Server uses Posix Threads
Client uses Select()


Functions with capital letters are Wrapper functions (Socket, Accept, Write,etc)
. They do error checking and are contained in Util.cpp.


If you find an error, I suggest quickly restarting the programs. I tried
to debug as much as possible. Rarely, you may get strange errors like
"broken pipe". Util.cpp may print errors. For example, if Read returns -1. Also,
please check your whitespace when inputting.


MAXBUFLEN, is set to 256, please adjust to send larger messages.
MAXBUFLEN, was used to verify that the entire message was transmitted.
I always read or wrote MAXBUFLEN-1, to leave space for '\0'.

When logging in, please don't enter extra space. For example, typing "alice"
vs "alice " will result in it not being recognized; or "password123" vs
" password123".


Initially, I began using mutexes to lock global variables. Then, I realized
race conditions rarely occured, so I stopped. However, please note that the
existing locks and mutexes are implemented correctly. (I tested for deadlocks
and didn't find any.)


If the client, receives concatenated messages, it will process only the first
and discard the rest. For example, "contactsOn: bob 0.0.0.0 51000; loggedIn:
alice 0.0.0.0 51000". "loggedIn" will be discarded.


Friends can re-invite themselves again. For example, Alice and Bob are already
friends, but they can still send each other "i"s and "ia"s.


Server changes the names on "i" and "ia"s to allow clients to know who the
message is coming from. For example, if Bob sends Alice, "i alice hello",
the server will send "i bob hello" to Alice, this way Alice knows that Bob
sent the message. This feature greatly simplifies "i"s and "ia"s.

When a new friend invites someone, they won't know if they are online or not.
For example, like on Facebook, before becoming friends, you don't know if the
other person is online or not.


Server will discard an "ia" for which there is no existing "i". This is to
prevent malicious "ia"s. For example, Alice can send an "ia"s to random people
and become everyone's friends otherwise.


Server Message Formats:
contactsOn: friend1 ip port; friend2 ip port;
loggedIn: friend ip port;
loggedOut: friend ip port;


For signal handling, the Server prints tids of threads killed and joined, and
also all the sockets closed. The client prints all the sockets closed.


For any questions, please don't hesitate to contact me.
