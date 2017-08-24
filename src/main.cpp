//
// miditester: a utility program which sends MIDI bytes 
//
// Copyright (C) 2017 Dan Wilcox <danomatika@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//
#include <iostream>
#include <cstdlib>
#include <chrono>
#include <thread>
#include "RtMidi.h"

// channel voice message     dec value, # data bytes
#define MIDI_NOTEOFF        0x80 // 128, 2
#define MIDI_NOTEON         0x90 // 144, 2
#define MIDI_POLYAFTERTOUCH 0xA0 // 160, 2, aka key pressure
#define MIDI_CONTROLCHANGE  0xB0 // 176, 2
#define MIDI_PROGRAMCHANGE  0xC0 // 192, 1
#define MIDI_AFTERTOUCH     0xD0 // 208, 1, aka channel pressure
#define MIDI_PITCHBEND      0xE0 // 224, 2

// system common message
#define MIDI_SYSEX          0xF0 // 240, variable, until MIDI_SYSEXEND
#define MIDI_TIMECODE       0xF1 // 241, 1
#define MIDI_SONGPOS        0xF2 // 242, 2
#define MIDI_SONGSELECT     0xF3 // 243, 1
//      MIDI_RESERVED1      0xF4 // 244, ?
//      MIDI_RESERVED2      0xF5 // 245, ?
#define MIDI_TUNEREQUEST    0xF6 // 246, 0
#define MIDI_SYSEXEND       0xF7 // 247, 0
 
// realtime message
#define MIDI_TIMECLOCK      0xF8 // 248, 0
//      MIDI_RESERVED3      0xF9 // 249, ?
#define MIDI_START          0xFA // 250, 0
#define MIDI_CONTINUE       0xFB // 251, 0
#define MIDI_STOP           0xFC // 252, 0
//      MIDI_RESERVED4      0xFD // 253, 0
#define MIDI_ACTIVESENSING  0xFE // 254, 0
#define MIDI_SYSTEMRESET    0xFF // 255, 0

static const char* HELP =
"Usage: miditester [OPTIONS] [TEST]\n"                    \
"\n"                                                      \
"  a utility program which sends MIDI bytes\n"            \
"\n"                                                      \
"Options:\n"                                              \
"  -p,--port  MIDI port to send to 0-n (default 0)\n"     \
"  -c,--chan  MIDI channel to send to 1-16 (default 1)\n" \
"  -s,--speed Millis between messages (default 500)\n"    \
"  -l,--list  List available MIDI ports and exit\n"       \
"  -h,--help  This help print\n"                          \
"\n"                                                      \
"TEST:\n"                                                 \
"  all        Run all tests below (default)\n"            \
"  channel    Channel messages  80 - E0\n"                \
"  system     System messages   F0 - F7\n"                \
"  realtime   Realtime messages F8 - FF\n"                \
"  running    Running status tests\n"                     \
"  sysex      Sysex tests\n"                              \
;

// convenience types
typedef std::vector<std::vector<unsigned char>> MessageQueue;
struct TestSet {
    std::string name;
    MessageQueue messages;
};
typedef std::vector<TestSet> TestQueue;

// returns true if a string has only numeric digits
bool isnumeric(const std::string &s) {
  return s.find_first_not_of("0123456789") == std::string::npos;
}

// add all message types to the queue
void channelMessages(TestQueue &queue, int channel=0);
void systemMessages(TestQueue &queue, int channel=0);
void realtimeMessages(TestQueue &queue, int channel=0);

// add specfic message type tests to the queue
void runningStatus(TestQueue &queue, int channel=0);
void sysex(TestQueue &queue, int channel=0);

// RtMidi error callback
void errorcallback(RtMidiError::Type type, const std::string &errorText, void *userData);

// actual program
int main(int argc, char *argv[]) {

    RtMidiOut *midiout = new RtMidiOut();
    midiout->setErrorCallback(errorcallback);
    TestQueue queue;

    // check if there is anything to send to
    unsigned int numPorts = midiout->getPortCount();
    if(numPorts == 0) {
        std::cout << "no ports available" << std::endl;
        delete midiout;
        return 0;
    }

    // parse commandline
    std::string tests = "all";
    int port = 0;
    int channel = 1;
    int speed = 500;
    bool list = false;
    std::string option = "";
    for(int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        // previous option which expects a value
        if(option != "") {
            if(arg[0] == '-') {
                std::cout << option << " expects a value" << std::endl;
                return 1;
            }
            if(!isnumeric(arg)) {
                std::cout << option << " expects a positive integer, got "
                          << arg << std::endl;
                return 1;
            }
            if(option == "-p" || option == "--port") {
                port = std::atoi(argv[i]);
            }
            else if(option == "-c" || option == "--channel") {
                channel = std::atoi(argv[i]);
                if(channel < 1 || channel > 16) {
                    std::cout << option << " option must be 1-16" << std::endl;
                    return 1;
                }
            }
            else if(option == "-s" || option == "--speed") {
                speed = std::atoi(argv[i]);
            }
            else {
                std::cout << "unknown option: " << option << std::endl;
                return 1;
            }
            option = "";
            continue;
        }

        // current option
        if(arg[0] == '-') {
            // option
            if(arg == "-h" || arg == "--help") {
                std::cout << HELP << std::endl;
                return 0;
            }
            else if(arg == "-l" || arg == "--list") {
                list = true;
                break;
            }
            else {
                option = arg;
            }
        }
        else {
            tests = arg;
        }      
    }

    // list devices and exit?
    if(list) {
        std::cout << "available ports:" << std::endl;
        for(int i = 0; i < numPorts; ++i) {
          std::cout << "  " << i << ": "
                    << midiout->getPortName(i) << std::endl;
        }
        return 0;
    }

    std::cout << "running tests: " << tests << std::endl
              << "port: " << port << std::endl
              << "channel: " << channel << std::endl
              << "speed: " << speed << " ms" << std::endl;
    channel--; // decrement from human-readable index

    // try opening given port
    midiout->openPort(port);
    std::cout << "opened " << midiout->getPortName(port) << std::endl;
    
    // prepare message queue
    bool alltests = (tests == "all");
    if(alltests || tests == "channel")  channelMessages(queue, channel);
    if(alltests || tests == "system")   systemMessages(queue, channel);
    if(alltests || tests == "realtime") realtimeMessages(queue, channel);
    if(alltests || tests == "running")  runningStatus(queue, channel);
    if(alltests || tests == "sysex")   sysex(queue, channel);

    // send messages
    std::chrono::milliseconds sleepMS(speed);
    for(auto &test : queue) {
        std::cout << test.name << " test" << std::endl;
        for(auto &message : test.messages) {
            std::cout << "  sending ";
            for(unsigned int byte : message) {
                std::cout << std::hex << std::uppercase
                          << byte
                          << std::nouppercase << std::dec << " ";
            }
            std::cout << std::endl;
            midiout->sendMessage(&message);
            std::this_thread::sleep_for(sleepMS);
        }
    }

    // cleanup
    midiout->closePort();
    delete midiout;
    return 0;
}

void channelMessages(TestQueue &queue, int channel) {
    TestSet set;
    set.name = "channel";
    set.messages = {

        {
            (unsigned char)(MIDI_NOTEON + channel),
            64, // note
            64  // velocity
        },
        
        {
            (unsigned char)(MIDI_NOTEOFF + channel),
            64, // note
            0   // velocity
        },

        {
            (unsigned char)(MIDI_POLYAFTERTOUCH + channel),
            64, // note
            64  // value
        },

        {
            (unsigned char)(MIDI_CONTROLCHANGE + channel),
            64, // control
            64  // value
        },

        {
            (unsigned char)(MIDI_PROGRAMCHANGE + channel),
            64  // program
        },

        {
            (unsigned char)(MIDI_AFTERTOUCH + channel),
            64  // value
        },

        {
            (unsigned char)(MIDI_PITCHBEND + channel),
            64, // value lsb
            0   // value msb
        }
    };
    queue.push_back(set);
}

void systemMessages(TestQueue &queue, int channel) {
    TestSet set;
    set.name = "system";
    set.messages = {

        {MIDI_SYSEX, 1, 2, 3, 4, MIDI_SYSEXEND},

        {
            MIDI_TIMECODE,
            10  // value
        },

        {
            MIDI_SONGPOS,
            20, // value 1
            30  // value 2
        },

        {
            MIDI_SONGSELECT,
            64  // song
        },

        {MIDI_TUNEREQUEST}
    };
    queue.push_back(set);
}

void realtimeMessages(TestQueue &queue, int channel) {
    TestSet set;
    set.name = "realtime";
    set.messages = {
        {MIDI_TIMECLOCK},
        {MIDI_START},
        {MIDI_CONTINUE},
        {MIDI_STOP},
        {MIDI_ACTIVESENSING},
        {MIDI_SYSTEMRESET}
    };
}

void runningStatus(TestQueue &queue, int channel) {
    TestSet set;
    set.name = "running";
    set.messages = {

        // start with note on
        {(unsigned char)(MIDI_NOTEON + channel), 64, 64},
        
        // note on without status byte
        {65, 64},

        // realtime messages should pass through 
        {MIDI_START},
        {66, 64},
        {MIDI_STOP},
        {67, 64},
        {MIDI_CONTINUE},
        {68, 64},
        {MIDI_TIMECLOCK},

        // note off
        {(unsigned char)(MIDI_NOTEOFF + channel), 64, 0},

        // note offs without status byte
        {64, 64},
        {65, 64},
        {66, 64},
        {67, 64},
        {68, 64}
    };
    queue.push_back(set);
}

void sysex(TestQueue &queue, int channel) {
    TestSet set;
    set.name = "sysex";
    set.messages = {

        // test realtime messages within sysex
        {MIDI_SYSEX, 5, MIDI_STOP, 6, MIDI_SYSEXEND},

        // test sysex without end sysex end byte
        // not all of these bytes may go through as MIDI
        // backends handle this in different ways
        //{MIDI_SYSEX, 7, 8, 9, 10, 11, 12, 13, 14},

        // next status message should work fine
        {(unsigned char)(MIDI_NOTEON + channel), 64, 64}
    };
    queue.push_back(set);
}

void errorcallback(RtMidiError::Type type, const std::string &errorText, void *userData) {
    std::cout << "RtMidi error: " << errorText << std::endl;
    std::exit(1);
}