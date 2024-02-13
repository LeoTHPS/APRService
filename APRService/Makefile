RM           ?= rm -f
CXX          ?= g++

SOURCE_FILES  = main.cpp
SOURCE_FILES += aprservice.cpp
SOURCE_FILES += aprservice_lua.cpp
SOURCE_FILES += aprservice_aprs.cpp
SOURCE_FILES += aprservice_events.cpp
SOURCE_FILES += aprservice_console.cpp
SOURCE_FILES += aprservice_commands.cpp
OBJECT_FILES  = $(SOURCE_FILES:.cpp=.o)

CXXFLAGS     += -Wall -Wfatal-errors -std=c++20 -I. -I"$(AL_INCLUDE)"

ifdef COMPILER
	ifeq ($(COMPILER), GNU)
		CXX = g++
	else ifeq ($(COMPILER), CLANG)
		CXX = clang++
	else
		CXX = $(COMPILER)
	endif
endif

ifdef PLATFORM
	ifeq ($(PLATFORM), LINUX)
		LDLIBS += -lpthread
		LDLIBS += -llua5.4
	endif

	ifeq ($(PLATFORM), WINDOWS)
		LDLIBS += -luser32
		LDLIBS += -lwsock32 -lws2_32 -lIphlpapi
		LDLIBS += -lshlwapi
		LDLIBS += -llua
	endif
endif

.PHONY: clean

APRService: $(OBJECT_FILES)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	$(RM) $(OBJECT_FILES)
