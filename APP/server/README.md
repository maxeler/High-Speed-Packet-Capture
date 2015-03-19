# Server
Receives capture data from a capture client and writes to a pcap file.

## Build
```console
$ make all
```

## Run
```console
$ ./capture_server --help
Usage: capture_server [OPTION...] [ip] file.pcap

  -v, --verbose=level
  -?, --help                 Give this help list
      --usage                Give a short usage message
```

```console
$ ./capture_server 5.5.5.3 ./capture.pcap
```
