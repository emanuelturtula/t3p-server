CC = g++
CFLAGS = -Wall

SOURCEPATH = \src

main: $(SOURCEPATH)\*.cpp $(SOURCEPATH)\*.h
	$(CC) $(CFLAGS) -o main.exe $(SOURCEPATH)\*.cpp

.PHONY: clean
clean:
	rm -rf *.o