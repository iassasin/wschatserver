CC = g++
CPPFLAGS = -Wall -O3 -std=c++1y
LDLIBS = -lmemcached

SOURCES = $(wildcard *.cpp)

APP_NAME = memcached_test
APP = $(APP_NAME)

all: $(APP)
	strip $(APP)

debug: CPPFLAGS = -D_DEBUG_ -Wall -g3 -std=c++1y
debug: $(APP)

clean:
	rm -f $(APP) $(SOURCES:%.cpp=%.o)

$(APP): $(SOURCES:%.cpp=%.o)
	$(LINK.o) $^ $(LDLIBS) -o $(APP)

