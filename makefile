CC = g++

INCLUDES = -I./redisclient/src -I./mysql_cppconn
CPPFLAGS = -Wall -O3 -std=c++20 -flto $(INCLUDES)
LDLIBS_SHARED = -lpthread -lcrypto -lmysqlcppconn -ljsoncpp -lssl -lpqxx -lpq
LDLIBS = $(LDLIBS_SHARED) -lboost_system

SOURCES = $(wildcard *.cpp) $(wildcard regex/*.cpp)
OBJECTS = $(SOURCES:%.cpp=%.o)

APP_NAME = wsserver
APP = $(APP_NAME)

all: $(APP)
	strip $(APP)

static: $(APP)-static
	strip $(APP)

static_boost: LDLIBS = -Wl,-Bstatic -lboost_system -Wl,-Bdynamic $(LDLIBS_SHARED)
static_boost: $(APP)
	strip $(APP)

debug: CPPFLAGS = -D_DEBUG_ -Wall -g3 -std=c++20 $(INCLUDES)
debug: $(OBJECTS)
	$(LINK.o) $^ $(LDLIBS) -o $(APP)

clean:
	rm -f $(APP) $(OBJECTS)

# Disable -flto for static build, because mysql exceptions cause sigsegv in core lib
# static linking just doesn't work because maintainers always forgetting to include static libs in deb-packages
$(APP)-static: LDLIBS = $(LDLIBS_SHARED) $(shell pkg-config --libs --static mysqlclient) $(shell pkg-config --libs --static libpq)
$(APP)-static: CPPFLAGS = -Wall -O3 -std=c++20 $(INCLUDES)
$(APP)-static: $(OBJECTS)
	$(LINK.o) -static $^ $(LDLIBS) -o $(APP)

$(APP): $(OBJECTS)
	$(LINK.o) -flto $^ $(LDLIBS) -o $(APP)
