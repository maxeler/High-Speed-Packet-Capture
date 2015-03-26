# High Speed Packet Capture
<img src="../github/icon.png" alt="High Speed Packet Capture" height="300" width="300"/>

## Description
Provides line-rate packet capture at bursts of up to 24GB in size. The application configures pairs of DFE SFP ports into “tapped” pass through connections. Ethernet data is captured from the tapped ports, packets are accurately time stamped and written out to pcap files. The DFE implementation can filter traffic at line rate and can also be extended to guarantee loss-less packet capture at 40GE and 100GE, given a sufficiently large storage back-end.


High Speed Packet Capture on [AppGallery](http://appgallery.maxeler.com/)

## Features
* Line-rate capture
* DFE based time stamping
* DFE LMEM buffer
* DFE packet filtering (broadcast vs non-broadcast traffic)
* Load-balanced writing of capture data
* Local writing of capture data

## To Do
* Passthrough ports
* Multi-port capture
* Dropped packet reporting

## Folder Structure

### ORIG
A "traditional" C CPU implementation of an ethernet packet capture program.

### SPLIT
ORIG has been split into code that will remain on the CPU and code that will eventually be implemented on a DFE. This is the first step in converting a "traditional" program over to a DFE.

### APP
A hybrid version of the original application which uses a DFE as a highspeed capture and buffer device, and a general purpose computer for long term persistence.
