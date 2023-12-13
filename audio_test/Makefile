CXX = aarch64-linux-gnu-g++
CXXFLAGS += -O2 -Wall -fPIC
CXXFLAGS += -I ./

LIBS  = -lpthread -lm -lstdc++ -ldl
LIBS += -L./ -ltinyalsa
LIBS += -L./ -lbsp_audio

CXXFLAGS += $(LIBS)

DIRS := $(shell find . -maxdepth 5 -type d)
SRC_FILES = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cpp))

OBJS  := $(SRC_FILES:%.cpp=%)

all: $(OBJS)

clean:
	$(RM) $(OBJS)