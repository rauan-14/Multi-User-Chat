## Multi user chat in C++ with the support of health checking and rooming
Simple multi user chat using socket programming in C++. Makefile is provided.
**server.cpp** -  server code.
**client.cpp** - client code.
**parsers.h** - code to parse incoming message and construct one.
**structs.h** - code containing structs to manage database, users and rooms.

### Messaging protocol used.
```
Type: Connect / Disconnect / GetUsers / ChangeRoom / ConnectionCheck / Error / Response / Message
Who: Server / Client
Name:
Room:
Receivers:
SleepTime:
Content:
```
Error is used to send error messages and sometimes as notifications.

Example:
```
Connect
Client
Rauan
10
Alice Bob
0
Hello World!
```
