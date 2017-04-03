## pvlib
Pvlib is s simple library to read data from pv inverters.
It supports reading dc, ac and status spot data and history data for day
yields and events.

It currently only supports sma inverters using the smadata2plus protocol.

## Installation

Install the dependencies:
- bluez
- cmake

On debian like system they can be installed by:
```sh
sudo apt-get install libluetooth cmake
```

To build the library
```sh
mkdir build
cmake ..
make
sudo make install
sudo ldconfig
```

## Usage
See example application pvlibshell.
