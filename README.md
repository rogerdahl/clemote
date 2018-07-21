# Clemote - IR Remote Control for Clementine Player

<img align="right" width="40%" src="./assets/remote-labels.jpg">

This app reads button presses from a remote control, sends commands to the Clementine Player, deletes and renames files, and updates song ratings.

The app controls Clementine via D-Bus. File and tag operations are performed directly, after which the Clementine library is synchronized in order to make the changes visible.

**Warning: The RECORD button immediately deletes the currently playing file from disk.**

The blue button adds ".delete" to the filename of the currently playing file but does not delete the file.

### Usage

* Find your remote control device with:

    `$ sudo apt install input-utils`
    `$ sudo lsinput`

* Start the Clementine Player
* Start the app with:

    `$ clemote remote </dev/input/eventX>`

If there's interest, I'll provide instructions for how to run the app as a service.

### Setup

* Allow the app to trigger refresh in Clementine to show changes:

    Clementine > Tools > Preferences > Music Library
        > Monitor library for changes > Enable

### Troubleshooting

* This app does not need LIRC, and LIRC may interfere if installed. Try:

    `$ apt remove lirc`

### Technologies

* sdbus-cpp - High level D-Bus C++ bindings
* taglib - Solid, battle tested, audio metadata library

### Build on Linux

Tested on Linux Mint 19.

Should work on Ubuntu 18.04 and other distributions based on it.

#### Packaged dependencies

    $ sudo apt install build-essential cmake \
    libboost-filesystem-dev libboost-system-dev \
    libsystemd-dev libpulse-dev libevdev-dev

#### sdbus-cpp

    $ git clone https://github.com/Kistler-Group/sdbus-cpp.git
    $ cd sdbus-cpp
    $ ./autogen.sh --disable-tests
    $ make -j$(nproc)

#### taglib

    $ git clone https://github.com/taglib/taglib.git
    $ cd taglib
    $ cmake -DCMAKE_BUILD_TYPE=Release .
    $ make -j$(nproc)
    $ sudo make install

### Troubleshoot D-Bus issues

    $ sudo apt install d-feet
    $ d-feet

