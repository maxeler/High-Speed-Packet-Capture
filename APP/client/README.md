# Client
Captures all traffic on QFSP_TOP_10G_PORT1. When local write is enabled traffic is written locally to a pcap file.
When remote write is enabled traffic is sent in a round-robin fashion to a pool of servers over QFSP_TOP_10G_PORT2. Broadcast traffic is sent to pool A and all other traffic is sent to pool B.

## Build
If you decide to build without the Makefile in `APP` be sure to gunzip the Maxfiles in `PLATFORMS` as the Maxfiles in `APP/client/RunRules/Maxfiles` are symlinked to the ones in `PLATFORMS`.

```console
$ make RUNRULE=... build
```

## Run
```console
$ cd RuneRules/RUNRULE/binaries
$ ./PacketCaptureClient --help                                                              
Usage: PacketCaptureClient [OPTION...] [dfe-ip dfe-netmask]

  -l, --local=pcap-file      Enable local write mode
  -r, --remote=type ip       Enable remote write mode
  -v, --verbose=level
  -?, --help                 Give this help list
      --usage                Give a short usage message

```

```console
$ ./PacketCaptureClient 5.5.5.2 255.255.255.0 -l capture.pcap -rA 5.5.5.3 -rA 5.5.5.4 -rB 5.5.5.5
```
