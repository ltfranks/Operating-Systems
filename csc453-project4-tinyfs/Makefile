CC = gcc
CFLAGS = -Wall -g -std=c99
LIBS = -lm

all: tinyFSDemo diskTest tfsTest

tinyFSDemo: tinyFSDemo.o libTinyFS.o libDisk.o
	$(CC) $(CFLAGS) -o tinyFSDemo tinyFSDemo.o libTinyFS.o libDisk.o $(LIBS)

diskTest: diskTest.o libTinyFS.o libDisk.o
	$(CC) $(CFLAGS) -o diskTest diskTest.o libTinyFS.o libDisk.o $(LIBS)

tfsTest: tfsTest.o libTinyFS.o libDisk.o
	$(CC) $(CFLAGS) -o tfsTest tfsTest.o libTinyFS.o libDisk.o $(LIBS)

tinyFSDemo.o: tinyFSDemo.c libTinyFS.h libDisk.h
	$(CC) $(CFLAGS) -c tinyFSDemo.c

diskTest.o: diskTest.c libTinyFS.h libDisk.h tinyFS_errno.h
	$(CC) $(CFLAGS) -c diskTest.c

tfsTest.o: tfsTest.c libTinyFS.h libDisk.h tinyFS_errno.h
	$(CC) $(CFLAGS) -c tfsTest.c

libTinyFS.o: libTinyFS.c libTinyFS.h libDisk.h tinyFS_errno.h
	$(CC) $(CFLAGS) -c libTinyFS.c

libDisk.o: libDisk.c libDisk.h
	$(CC) $(CFLAGS) -c libDisk.c

clean:
	rm -f *.o tinyFSDemo diskTest tfsTest
