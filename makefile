CC = g++
CPPFLAGS = -Wall -O3 -std=c++1y -flto
LDLIBS = -lpthread -lboost_system -lcrypto -lmysqlcppconn -lmemcached

SOURCES = $(wildcard *.cpp)
OBJECTS = $(SOURCES:%.cpp=%.o)

APP_NAME = wsserver
APP = $(APP_NAME)

all: $(APP)
	strip $(APP)

debug: CPPFLAGS = -D_DEBUG_ -Wall -g3 -std=c++1y
debug: $(APP)

clean:
	rm -f $(APP) $(OBJECTS)

$(APP): $(OBJECTS)
	$(LINK.o) -flto $^ $(LDLIBS) -o $(APP)

