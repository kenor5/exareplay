# Exareplay
Exareplay is an utility for replaying pcap which was previously captured by toos like tcpdump and Wireshark.
Exareplay needs to be used with Exanic NIC.
It provides high-precision packet replaying, with fluctuation of less than 100ns.

# Dependencies
```
libpcap
libexanic
```

# Build and install
TODO

# Usage
```
[sudo] exareplay
  -r <input_file>    input file name
  -i <device:port>   NIC device and port, for example 'exanic0:0
  -d                 replay two file at the same time
  -s <interval>      skip large time interval in pcap
```