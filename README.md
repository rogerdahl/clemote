# Clemote - IR Remote Control for Clementine Player

<img align="right" width="40%" src="./assets/remote-labels.jpg">

This app reads button presses from a remote control, sends commands to the Clementine Player, deletes and renames files, and updates song ratings.

The app controls Clementine via D-Bus. File and tag operations are performed directly, after which the Clementine library is synchronized in order to make the changes visible.

**Warning: The RECORD button immediately deletes the currently playing file from disk.**

The blue button adds ".delete" to the filename of the currently playing file but does not delete the file.

### Usage

* Find your remote control device with:

    `$ lsinput`

* Start the Clementine Player
* Start the app with:

    `$ clemote remote </dev/input/eventX>`

If there's interest, I'll provide instructions for how to run the app as a service.

### Setup

* Allow the app to trigger refresh in Clementine to show changes:

    `Clementine > Preferences > Music Library > Monitor library for changes > Enable`

### Troubleshooting

* This app does not need LIRC, and LIRC may interfere if installed. Try:

    `$ apt remove lirc`

### Technologies

    * sdbus-cpp - High level D-Bus C++ bindings
    * taglib - Solid, battle tested, audio metadata library

### Build on Linux

#### gcc

Need gcc >= 6 for sdbus-cpp

Follow: https://solarianprogrammer.com/2016/10/07/building-gcc-ubuntu-linux/

#### sdbus-cpp

Building with local gcc

    # export CC=/usr/local/gcc-7.3/bin/gcc-7.3
    # export CXX=/usr/local/gcc-7.3/bin/g++-7.3
    # export LD_LIBRARY_PATH=/usr/local/gcc-7.3/lib64/
    $ sudo apt install libsystemd-dev
    $ export CPPFLAGS=-DDEBUG
    $ export CXXFLAGS="-g -O0"
    $ ./autogen.sh CC=/usr/local/gcc-7.3/bin/gcc-7.3 CXX=/usr/local/gcc-7.3/bin/g++-7.3 LD_LIBRARY_PATH=/usr/local/gcc-7.3/lib64/gcc/x86_64-linux-gnu/7.3.0/
    --disable-tests
    $ make -j

#### taglib

    $ cmake -DCMAKE_INSTALL_PREFIX=./mytaglib -DCMAKE_BUILD_TYPE=Release .
    $ make
    $ sudo make install
