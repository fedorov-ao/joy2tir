CC = i686-w64-mingw32-g++-win32
TARGET = NPClient.dll
HEADERS = NPClient.hpp logging.hpp joystick.hpp
SOURCES = NPClient.cpp logging.cpp joystick.cpp
OBJECTS = $(SOURCES:%.cpp=%.o)
#If compiled with -On, dll can not be loaded
CFLAGS = -std=c++11 -I. -D_WIN32_WINNT=0x0501 -DNDEBUG -Os -ffunction-sections -fdata-sections
LDFLAGS = -static-libstdc++ -static-libgcc -shared -s -Wl,--gc-sections,--exclude-all-symbols,--kill-at,-lwinmm
INSTALL_PATH = ./bin

TEST_TARGET = joystick_test.exe
TEST_HEADERS = logging.hpp joystick.hpp
TEST_SOURCES = joystick_test.cpp logging.cpp joystick.cpp
TEST_OBJECTS = $(TEST_SOURCES:%.cpp=%.o)
TEST_CFLAGS = -std=c++11 -I. -D_WIN32_WINNT=0x0501 -DNDEBUG -Os -ffunction-sections -fdata-sections
TEST_LDFLAGS = -static-libstdc++ -static-libgcc -s -Wl,--gc-sections,--exclude-all-symbols,--kill-at,-lwinmm

%.o: %.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c $*.cpp

all: release test

release: $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

test: $(TEST_OBJECTS)
	$(CC) $(TEST_CFLAGS) -o $(TEST_TARGET) $(TEST_OBJECTS) $(TEST_LDFLAGS)

install:
	mkdir $(INSTALL_PATH)
	cp $(TARGET) $(INSTALL_PATH)

uninstall:
	rm $(INSTALL_PATH)/$(TARGET) 

clean:
	rm  *.o *.def *.lib *.dll 2>1
