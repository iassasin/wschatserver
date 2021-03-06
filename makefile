CC = g++

INCLUDES = -I./redisclient/src -I./mysql_cppconn
CPPFLAGS = -Wall -O3 -std=c++17 -flto $(INCLUDES)
LDLIBS = -lpthread -lboost_system -lcrypto -lmysqlcppconn -ljsoncpp -lssl

SOURCES = $(wildcard *.cpp) $(wildcard regex/*.cpp)
OBJECTS = $(SOURCES:%.cpp=%.o)

APP_NAME = wsserver
APP = $(APP_NAME)

all: $(APP)
	strip $(APP)

static: $(APP)-static
	strip $(APP)

static_boost: LDLIBS = -lpthread -Wl,-Bstatic -lboost_system -Wl,-Bdynamic -lcrypto -lmysqlcppconn -ljsoncpp -lssl
static_boost: $(APP)
	strip $(APP)

debug: CPPFLAGS = -D_DEBUG_ -Wall -g3 -std=c++17 $(INCLUDES)
debug: $(OBJECTS)
	$(LINK.o) $^ $(LDLIBS) -o $(APP)

clean:
	rm -f $(APP) $(OBJECTS)

# Disable -flto for static build, because mysql exceptions cause sigsegv in core lib
$(APP)-static: LDLIBS = -lpthread -lboost_system -lcrypto -ljsoncpp -lssl -lmysqlcppconn $(shell pkg-config --libs --static mysqlclient)
$(APP)-static: CPPFLAGS = -Wall -O3 -std=c++17 $(INCLUDES)
$(APP)-static: $(OBJECTS)
	$(LINK.o) -static $^ $(LDLIBS) -o $(APP)

$(APP): $(OBJECTS)
	$(LINK.o) -flto $^ $(LDLIBS) -o $(APP)
