[![Build Status](https://travis-ci.org/p4lang/p4c.svg?branch=master)](https://travis-ci.org/p4lang/p4c)

# p4box

p4box is a system for deploying runtime monitors in programmable data planes. It extends the P4_16 prototype compiler.

# Getting started

1. Clone the repository. It includes submodules, so be sure to use `--recursive` to pull them in:
```
  git clone --recursive https://github.com/mcnevesinf/p4box.git
```
If you forgot `--recursive`, you can update the submodules at any time using:
```
  git clone --recursive https://github.com/mcnevesinf/p4box.git
```

2. Install dependencies (p4box was tested on Ubuntu 16.04).
 
  Most dependencies can be installed using `apt-get install`:
```
sudo apt-get install cmake g++ git automake libtool libgc-dev bison flex libfl-dev libgmp-dev libboost-dev libboost-iostreams-dev libboost-graph-dev llvm pkg-config python python-scapy python-ipaddr python-ply tcpdump
```
  An exception is Google Protocol Buffers; `p4box` depends on version 3.0 or higher, which is not available until Ubuntu 16.10. For earlier releases of Ubuntu, you'll need to install from source. You can find instructions [here](https://github.com/protocolbuffers/protobuf/blob/master/src/README.md). We recommend that you use version 3.2.0. After cloning protobuf and before you build, check-out version 3.2.0.
```
  git checkout v3.2.0
```

3. Build. Building should also take place in a subdirectory named `build`.

```
mkdir build
cd build
cmake ..
make -j4
```

4. Install `p4box` and its shared headers globally.
```
  sudo make install
```

5. You're ready to go! See _examples/_ to start experimenting with `p4box`.

More details coming soon.

