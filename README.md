# cstorage

cstorage is a pastebin clone.

## Build & Run

You will need `make` and `clang-12` to build it. You will also need `python3`, `netcat-traditional`,`uuid-runtime` to
run tests.

The easiest way to run tests is to run them via docker:

```sh
make test-docker
```

## Examples

### Start server

```sh
$ ./cstorage 8080 servdir
[INFO] serving on port 8080 with folder servdir
[INFO] [connection 1] new connection
[INFO] [connection 1] called set
[INFO] [connection 1] save data to disk
[INFO] saved data to file ./servdir/6oh9sa1obn6z90oqg06r3ohlfnxstwhzkyos
[INFO] [connection 1] saved
[INFO] [connection 1] response is sent, closing
```

### Paste file using client

```sh
$ ./cstorage-client 127.0.0.1 8080 set example.txt
d8f7dxhqjq3gqu3ct43v6djw3qm6b264alcn
```

### Download file using client

```sh
$ ./cstorage-client 127.0.0.1 8080 get d8f7dxhqjq3gqu3ct43v6djw3qm6b264alcn
#include <stdio.h>

int main() {
  printf("Hello World!\n");
}
```

## Protocol

cstorage uses a simple protocol over TCP.

### SET

#### Request

- First line contains `SET` header;
- Second line contains 8 characters with hexadecimal content-length;
- Then the file content.

#### Response

- Single line -- 36 characters of paste id.

#### Example using netcat as a client

```sh
$ cat set-request-example.txt
SET
0000000D
Hello, World!
$ cat set-request-example.txt | nc localhost 8080
oph0d3f233tj4fqn47ozb1rkxpjy3nqssns5
```

### GET

#### Request

- First line contains `GET` header;
- Second line contains 36 characters of paste id;

#### Response

- File content.

#### Example using netcat as a client

```sh
$ cat get-request-example.txt
GET
oph0d3f233tj4fqn47ozb1rkxpjy3nqssns5
$ cat get-request-example.txt | nc localhost 8080
Hello, World!
```

## Details

cstorage

- is single-threaded;
- uses `epoll(7)` for network communication;
- uses blocking disk i/o;
- borrows some code from [this](https://eli.thegreenplace.net/2017/concurrent-servers-part-3-event-driven/) blog.

The entrypoints are:

- `main.c` for server
- `client.c` for client
