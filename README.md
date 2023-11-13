#Exareplay
Exareplay is an utility for replaying pcap which was previously captured by toos like tcpdump and Wireshark.
Exareplay needs to be used with Exanic NIC.
It provides high-precision packet replaying, with fluctuation less than 100ns.

Exareplay is much more than a simple packet replay application. Some of the Exareplay features includes:

* High-precision. 95% of time interval fluctuate within 100ns.
* Fully user configurable.
* Use of the standard PCAP file format (regular and nanoseconds).
* Line-rate packet replay.
* Inter-packet time tuning. Exareplay is able to replay traffic at capture rate or at a user-defined rate.
* Multi-core support. Exareplay has been designed with multicore architectures in mind. It uses 3 threads (one for reading packets from disk, one for loading packets from memory to NIC buffer and one for precise time control). 
* PF_RING acceleration. Exareplay exploit the packet transmission acceleration offered by PF_RING ZC.
* Direct-IO disk access. Exareplay uses the Direct IO access to the disks in order to obtain maximum disk-read throughput.

#Dependencies
```
libpcap
libexanic
```

#Build and install
TODO

#Usage
```
[sudo] exareplay
  -r <input_file>    input file name
  -i <device:port>   NIC device and port, for example 'exanic0:0'
  -d                 replay two file at the same time, **not supported yet**
  -s <interval>      skip large time interval in pcap
  -m <memory>        memory to use, default 1G
  -c <pcap_cnt>      num to replay, default all
```