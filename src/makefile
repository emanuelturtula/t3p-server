CC = g++
CFLAGS = -Wall

ALL_CC_FILES = main.cc ErrorHandler.cc LogRecords.cc SocketsSetter.cc 
ALL_HH_FILES = main.h  ErrorHandler.h  LogRecords.h  SocketsSetter.h 

main: $(ALL_CC_FILES) $(ALL_HH_FILES)
	$(CC) $(CFLAGS) -o main.exe $(ALL_CC_FILES)

.PHONY: clean
clean:
	rm -rf *.o