# APP
The client program is the core of the packet capture application and serves two purposes. One is to configure the DFE with the packet capture implementation. Second is to read data from the DFE and write capture data to a pcap file when local capture is enabled. When local capture has not been enabled the binary simply holds onto the DFE to inform MaxelerOS that it is still in use.
In local capture mode the DFE captures traffic on QSFP_TOP_PORT1 and transfers the traffic via PCIe to the CPU.
In remote capture mode the DFE captures traffic on QSFP_TOP_PORT1 and sends traffic in a roundrobin fashion over QSFP_TOP_PORT2 to each server in a server pool. Broadcast traffic is sent to pool A and all other traffic is sent to pool B.

To run on hardware you will need a supported data flow engine and a computer running MaxelerOS.

## Known Issues
* Some captured traffic will contain extra trailing zeros when run in simulation
* Sanity check of capture data occasionally fails in simulation

## Building
For convenience the server, client, and common files can be built with the Makefile in APP.

```shell
cd APP

## Simulation
make all-simulation
## Isca
make all-isca
```

## Running
### Simulation
When running in simulation the hardware is simulated by maxelerossim and the virtual network is managed by maxethsimd.

The easiest way to start maxethsimd is via a [MaxIDE](https://www.maxeler.com/products/software/maxcompiler/) run rule so let's first import the client into the development environment. `File > Import...` then select `General > MaxCompiler Projects into Workspace`. The root directory will be located in `APP/client`. Make sure the "PacketCapureClient" project is checked and click the `Finish` button.

<img src="../../github/screenshots/dialog_import_maxcompiler.png"/>

We will be running the "SIMULATION" run rule in MaxIDE. Run rules are located in the Workspace tab within your project under the "Run Rules" folder.

<img src="../../github/screenshots/workspace_runrules.png"/>

Double-click the "SIMULATION" run rule to bring up the run rule window. The "Start/Stop Simulated System Manually" check-box should be checked. Click the "Start" button to start the simulated network.

<img src="../../github/screenshots/runrule_simulator_tab.png"/>

You will notice that when a network based simulated system is running some virtual network interfaces will appear. You should see one per simulated DFE interface.

```console
$ ifconfig | grep tap
tap0      Link encap:Ethernet  HWaddr B8:CD:A7:FF:FF:00
tap1      Link encap:Ethernet  HWaddr B8:CD:A7:FF:FF:01
```
Note: When maxethsimd is no longer running these taps will disappear.

tap0 and tap1 represent the QSFP_TOP_10G_PORT1 and QSFP_TOP_10G_PORT2 interfaces respectively.
In this design network data is captured on QSFP_TOP_10G_PORT1 and server data is transmitted on QSFP_TOP_10G_PORT2.

The script `APP/server/start_server.sh` does everything necessary to start a few locally running capture servers which work with a locally running capture client started with the unmodified "SIMULATION" run rule.

```console
$ cd APP/server
$ ./start_servers.sh
Configuring interfaces
    inet 5.5.5.1/24 brd 5.5.5.255 scope global tap1
    inet 5.5.5.3/24 scope global secondary tap1
    inet 5.5.5.4/24 scope global secondary tap1
    inet 5.5.5.5/24 scope global secondary tap1
    inet6 fe80::bacd:a7ff:feff:ff01/64 scope link
Creating server for 5.5.5.3
Creating server for 5.5.5.4
Creating server for 5.5.5.5
(Press enter to exit)
5.5.5.3: Waiting for client...
5.5.5.5: Waiting for client...
5.5.5.4: Waiting for client...
```

Finally we are ready to start the client. In MaxIDE the run rule toolbar allows you to build, debug launch, and launch a project based on the selected project and run rule. Make sure you have selected the 'PacketCaptureClient' project and selected the 'SIMULATION' run rule. Click the run button (octagon with white "play" symbol in the center) to start the client.

<img src="../../github/screenshots/toolbar_runrule.png"/>

This will trigger a compile of the CPUCode using the pre-built Maxfile supplied in PLATFORMS/Simulation. Once this is complete the PacketCaptureClient will begin running on the simulated system.

<img src="../../github/screenshots/console_simulation_run.png">

You should see each server receive a connection from the client.
```console
# ...
5.5.5.3: Client connected 5.5.5.2: 47292.
5.5.5.4: Client connected 5.5.5.2: 47709.
5.5.5.5: Client connected 5.5.5.2: 48157.
```

You might see some spurious traffic on tap0 being captured. If not we can generate our own traffic with a ping to a non-existent address on the same subnet as tap0. This will generate some broadcast traffic which will be sure to be seen by the DFE on tap0.

```shell
$ ping 172.0.0.2
PING 172.0.0.2 (172.0.0.2) 56(84) bytes of data.
From 172.0.0.1 icmp_seq=2 Destination Host Unreachable
From 172.0.0.1 icmp_seq=3 Destination Host Unreachable
From 172.0.0.1 icmp_seq=4 Destination Host Unreachable
# ...
```

You should see the servers and client report that packets have been captured.

Servers:
```
5.5.5.3: Total packets: 1
5.5.5.4: Total packets: 1
5.5.5.3: Total packets: 2
5.5.5.5: Total packets: 1
5.5.5.5: Total packets: 2
5.5.5.4: Total packets: 2
5.5.5.5: Total packets: 3
5.5.5.5: Total packets: 4
```

Client:
```
Fri 15:45: 5.5.5.2: Total packets: 1
Fri 15:45: 5.5.5.2: Total packets: 2
Fri 15:45: 5.5.5.2: Total packets: 3
Fri 15:45: 5.5.5.2: Total packets: 4
Fri 15:45: 5.5.5.2: Total packets: 5
Fri 15:45: 5.5.5.2: Total packets: 6
Fri 15:45: 5.5.5.2: Total packets: 7
Fri 15:45: 5.5.5.2: Total packets: 8
Fri 15:45: 5.5.5.2: Total packets: 9
```

Traffic received by the client will be in `APP/client/CPUCode/capture.pcap`. Traffic received remotely by the servers will be in .pcap files in the directory `APP/server/captures/`. These files can be opened with [Wireshark](https://www.wireshark.org/) or any other standard pcap file reader.

## DFE: ISCA
Make sure MaxelerOS is running on the computer the client will be running on and that the DFE is available.

```shell
$ maxtop
MaxTop Tool 2014.2
Found 1 Maxeler card(s) running MaxelerOS 2014.2
Card 0: Isca (P/N: ******) S/N: ********* Mem: 24GB Net: x2

Load average: 0.00, 0.00, 0.00

DFE  %BUSY  TEMP   MAXFILE        PID    USER       TIME      COMMAND         
 0   0.0%   50.5C  IDLE (r7)      -      -          -         -       
```

Ensure that the capture port QSFP_TOP_10G_PORT1 is connected to the source you want to capture and the link is up.

```shell
$ maxnet link show QSFP_TOP_10G_PORT1
QSFP_TOP_10G_PORT1:
            Type: QSFP                  
          Vendor: Molex Inc.            
   Part No / Rev: 747649108 / B         
 RX Signal (SFP): true                  
 TX Signal (SFP): true                  
         Link Up: true                  
     MAC address: b8:cd:a7:02:02:60     
      RX Enabled: true                  
       RX Frames: 0 ok                  
                  0 error               
                  0 CRC error           
                  0 invalid/errored     
                  0 total               
      TX Enabled: true                  
       TX Frames: 0 ok                  
                  0 error               
                  0 CRC error           
                  0 invalid/errored     
                  0 total               
```

#### Local Capture
Start the client in local capture mode.
```console
$ ./PacketCaptureClient -l PCAP-FILE
```

You should see the client report that packets have been captured.

#### Remote Capture
Before the client can be started the servers must started. Be sure the server's network is accessible by the client via TCP over port 2511. Start the server with the ip it should bind to and the capture file traffic will be written to. Take note of the servers' ip addresses. You will need them later to run the client.
```console
./PacketCaptureServer IP PCAP-FILE

```

It is a good idea to confirm that the servers are reachable from the DFE. This can be accomplished with `maxping`.

```console
$ maxping QSFP_TOP_10G_PORT2 DFE_NETMASK DFE_IP SERVER_IP
PING DFE_IP 56(84) bytes of data.
64 bytes from DFE_IP: icmp_seq=1 ttl=64 time=0.038 ms
# ...
```

Once all servers are running the client is ready to be started with the list of servers it will send capture data to. Remote servers are specified with the `-r` flag followed by the server pool and the server ip. The DFE will send all broadcast traffic to pool A and all other captured traffic to pool B.

```console
/PacketCaptureClient -rA POOLA_IP1 -rB POOLB_IP1 -rA POOLA_IP2 -rB POOLB_IP2 ...
```

You should see the client report that a connection has been established to each of the servers and the servers should report that a client has connected. The servers will report the total number of packets received as data is sent to them.
