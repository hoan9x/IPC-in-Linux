This project demonstrates Inter-Process Communication (IPC) using Unix domain sockets.
It includes two main examples: one for one-to-one communication between a single server and a client, and another for one-to-many communication where a server handles multiple clients concurrently.

## Directory structure

```
|- script/
|  |- build.sh              # Script to build the executable files
|
|- README.md                # This README file
|
|- output_build/
|  |- server.app               # Executable for the one-to-one server
|  |- client.app               # Executable for the one-to-one client
|  |- multiplexing_server.app  # Executable for the one-to-many server using select()
|  |- multiplexing_server2.app # Executable for the one-to-many server using pselect()
|  |- multiplexing_server3.app # Executable for the one-to-many server using poll()
|  |- many_client.app          # Executable for the one-to-many client
|
|- one_to_many/
|  |- client.c              # Source code for one-to-many client
|  |- server.c              # Source code for one-to-many server using select()
|  |- server2.c             # Source code for one-to-many server using pselect()
|  |- server3.c             # Source code for one-to-many server using poll()
|
|- one_to_one/
|  |- client.c              # Source code for one-to-one client
|  |- server.c              # Source code for one-to-one server
```

## Build instructions

Run the build script: The `script/build.sh` will compile the source code and place the executables into the `output_build/` directory.

## Running the examples

### One-to-One IPC example

This example demonstrates communication between one server and one client using Unix domain sockets.

Open two terminals:
+ Terminal 1 (Server): Run the server executable:

```bash
./output_build/server.app
```

+ Terminal 2 (Client): Run the client executable:

```bash
./output_build/client.app
```

The client will send data to the server over the Unix Domain Socket, and the server will process the data and return a response.

### One-to-Many IPC example

This example demonstrates a server handling multiple clients simultaneously using multiplexing `select()`, `pselect()`, and `poll()`.

Open multiple terminals:
+ Terminal 1 (Server): Run the multiplexing server executable:

```bash
./output_build/multiplexing_server.app
# OR
./output_build/multiplexing_server2.app
# OR
./output_build/multiplexing_server3.app
```

+ Terminal 2...n (Clients): In each terminal, run the client executable:

```bash
./output_build/many_client.app
```

The server can handle multiple clients at the same time. Each client can connect, send data concurrently.
