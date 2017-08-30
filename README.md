miditester
==========

a utility program which sends and receives MIDI bytes 

Copyright (C) 2017 Dan Wilcox <danomatika@gmail.com>

Description
-----------

When working with MIDI software projects, it's often useful to be able to generate and print incoming MIDI messages to make sure MIDI input handling is working as expected. This small commandline utility provides this functionality by either listening for messages or sending a series of test messages based on the required message type.

The project utilizes [RtMidi library](http://www.music.mcgill.ca/~gary/rtmidi/) for MIDI communication.

Building
--------

This is a simple commandline utility and should work on macOS, Linux, & Windows.

Build requirements:

* C++ compiler: gcc or clang
* GNU make

For building on Windows, [Msys2](http://www.msys2.org) can provide the necessary shell environment and compiler chain.

On Linux, you will need the development files for ALSA.

Build miditester using make:

    make

Usage
-----

By default, miditester opens the first MIDI port, sends all output test messages, and uses channel 1 for channel-based messages.

    ./miditester

To listen for incoming messages, use the `input` test argument:

    ./miditester input

All available options are listed in the help output using the `-h` or `--help` flags:

~~~
Usage: miditester [OPTIONS] [TEST]

  a utility program which sends and receives MIDI bytes

Options:

  -p,--port    MIDI port to use 0-n, default 0
  -c,--chan    MIDI channel to send to 1-16, default 1
  -s,--speed   Millis between messages,
               defaults: input 40, output 500
  -d,--decimal Print decimal byte values instead of hex
  -n,--name    Print status byte name instead of value
  -l,--list    List available MIDI ports and exit
  -h,--help    This help print

TEST:

  input    Listen & print MIDI messages

  all      Run all output tests below, default

  channel  Channel messages  80 - E0
  system   System messages   F0 - F7
  realtime Realtime messages F8 - FF
  running  Running status tests
  sysex    Sysex tests
  timecode Timecode tests: quarter & full frame
~~~

For example, to choose a specific MIDI port, first use the `-l` or `--list` flag which prints the available ports:

~~~
./miditester --list
input ports:
  0: IAC Driver Pure Data In
  1: IAC Driver Pure Data Out
output ports:
  0: IAC Driver Pure Data In
  1: IAC Driver Pure Data Out
~~~

You can then specify the port number with the `-p` or `--port` flags:

    ./miditester --port 1

To choose a specific test set, add the optional test argument:

    ./miditester --port 1 realtime
