#
# Copyright (c) 2017 Dan Wilcox <danomatika@gmail.com>
#

# detect platform
UNAME = $(shell uname)
ifeq ($(UNAME), Darwin) # Mac
    PLATFORM = mac
    CXXFLAGS = -D__MACOSX_CORE__
    AUDIO_API = -framework Foundation -framework CoreAudio -framework CoreMidi
else
    ifeq ($(OS), Windows_NT) # Windows, use Mingw
        PLATFORM = windows
        CXXFLAGS = -D__WINDOWS_MM__
        AUDIO_API = -lole32 -loleaut32 -ldsound -lwinmm
    else  # assume Linux
        PLATFORM = linux
        CXXFLAGS = -D__LINUX_ALSA__
        AUDIO_API = -lamidi -pthread
        # alsa flags: -D__UNIX_JACK__ -ljack
    endif
endif

SRC_FILES = src/main.cpp src/RtMidi.cpp
TARGET = miditester

CXXFLAGS += -I./src -std=c++11 -O3

.PHONY: clean clobber

$(TARGET): ${SRC_FILES:.cpp=.o} $(LIBPD)
    g++ -o $(TARGET) $^ $(AUDIO_API)

clean:
    rm -f src/*.o

clobber: clean
    rm -f $(TARGET)
