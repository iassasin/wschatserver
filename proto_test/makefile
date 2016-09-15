CC = g++
CPPFLAGS = -Wall -O3 -std=c++1y
LDLIBS = 

SOURCES = $(wildcard *.cpp) ../proto.cpp

APP_NAME = proto_test
APP = $(APP_NAME)

all: $(APP)
	strip $(APP)

debug: CPPFLAGS = -D_DEBUG_ -Wall -g3 -std=c++1y
debug: all

clean:
	rm -f $(APP) $(SOURCES:%.cpp=%.o)

$(APP): $(SOURCES:%.cpp=%.o)
	$(LINK.o) $^ $(LDLIBS) -o $(APP)

