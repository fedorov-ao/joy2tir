CC = i686-w64-mingw32-g++-win32
TARGET = NPClient.dll
HEADERS = NPClient.hpp
SOURCES = NPClient.cpp
OBJECTS = $(SOURCES:%.cpp=%.o)
#If compiled with -On, dll can not be loaded
CFLAGS = -std=c++11 -I. -D_WIN32_WINNT=0x0501 -DNDEBUG -Os -ffunction-sections -fdata-sections
LDFLAGS = -static-libstdc++ -static-libgcc -shared -s -Wl,--gc-sections,--exclude-all-symbols,--kill-at -ldxguid
INSTALL_PATH = ./bin

%.o: %.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c $*.cpp

all: debug

debug: $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

install:
	mkdir $(INSTALL_PATH)
	cp $(TARGET) $(INSTALL_PATH)

di8_uninstall:
	rm $(INSTALL_PATH)/$(TARGET) 

clean:
	rm  *.o *.def *.lib *.dll 2>1
