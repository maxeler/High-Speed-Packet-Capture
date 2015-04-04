# Capture
Captures all traffic on the specified interface and writes it locally to a pcap file.

## Build
```console
$ make
```

## Run
```console
./client --help
Usage: capture [OPTION...] interface file.pcap

  -v, --verbose=level        Set log level
  -?, --help                 Give this help list
      --usage                Give a short usage message

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponding short options.
```

```console
$ ./capture eth0 capture.pcap
```
