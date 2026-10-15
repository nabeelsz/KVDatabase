# Overview
This is a **work-in-progress** implementation of an in-memory key-value store written in C++, built without the STL, Boost, or other high-level library abstractions. The reason for this is for stronger runtime and memory guarantees, better control over every layer, and personal learning. 

## Progress So Far (As Of 10/14/2026)
The database currently is multithreaded and can support concurrent clients doing *PUT* and *GET* operations. Operations support keys up to 256 bytes and values up to 256 bytes. The spaces inbetween are necessary. 

A PUT operation supports a key and a value. If the value is a string or character, it needs a null terminator as shown below: 
```text
PUT <key> <value>\0
```
Otherwise, a non-string or character value uses a newline. 
```text
PUT <key> <value>\n
```
Please note that keys up to 256 bytes and values up to 256 bytes are supported. The null terminator in the value does not count towards this 256 byte limitation. If there are any values that are greater than this, the operation is deemed as invalid and unsuccessful. If the format is respected, the operation is completed, and a response message indicating success is sent to the client. 

A GET operation is described in the format below: 
```text
GET <key>\0
```
The GET operation is deemed invalid if the key is too large or if the key does not exist within the database. If successful, it returns the value (in bytes), so the client can utilize the value correctly. 

The database currently is implemented under the hood through a custom implementation of a hash-map that uses a pre-allocated pool of nodes. On top is the class for the database that uses mutexes to avoid race conditions. Lastly, communication between clients is done through a backend that uses POSIX sockets alongside a TCP protocol. 

## Current Limitations
- Supporting PUTS for the same key to map to one node rather than have multiple, e.g, currently a PUT for "key" may correlate to 
mutliple values and this is hard to follow. 
- More robust error handling, regarding giving an error message and hanging up on the client's socket, rather than ignoring the invalid request, not giving a response, yet not closing the socket as well. 
- Sending over the size of the value as well, and not just the value
