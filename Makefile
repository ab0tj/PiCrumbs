C=g++
CFLAGS=-c -Wall
OBJS=beacon.o console.o gps.o hamlib.o http.o init.o main.o pi.o predict.o stringfuncs.o tnc.o INIReader.o

all: picrumbs

picrumbs: $(OBJS)
	$(C) $(OBJS) -lpthread -lhamlib -lhamlib++ -lwiringPi -lcurl -lgps -o picrumbs

beacon.o: beacon.cpp beacon.h hamlib.h pi.h http.h predict.h tnc.h
	$(C) $(CFLAGS) -o beacon.o beacon.cpp

console.o: console.cpp console.h version.h stringfuncs.h
	$(C) $(CFLAGS) -o console.o console.cpp

gps.o: gps.cpp gps.h
	$(C) $(CFLAGS) -o gps.o gps.cpp

hamlib.o: hamlib.cpp hamlib.h beacon.h
	$(C) $(CFLAGS) -o hamlib.o hamlib.cpp

http.o: http.cpp http.h version.h
	$(C) $(CFLAGS) -o http.o http.cpp

init.o: init.cpp init.h INIReader.h version.h hamlib.h stringfuncs.h
	$(C) $(CFLAGS) -o init.o init.cpp

main.o: main.cpp main.h beacon.h gps.h tnc.h console.h init.h
	$(C) $(CFLAGS) -o main.o main.cpp

pi.o: pi.cpp pi.h beacon.h
	$(C) $(CFLAGS) -o pi.o pi.cpp

predict.o: predict.cpp predict.h beacon.h
	$(C) $(CFLAGS) -o predict.o predict.cpp

stringfuncs.o: stringfuncs.cpp stringfuncs.h
	$(C) $(CFLAGS) -o stringfuncs.o stringfuncs.cpp

tnc.o: tnc.cpp tnc.h beacon.h stringfuncs.h
	$(C) $(CFLAGS) -o tnc.o tnc.cpp

INIReader.o: INIReader.cpp INIReader.h ini.c ini.h
	$(C) $(CFLAGS) -o INIReader.o INIReader.cpp

clean:
	rm -f picrumbs $(OBJS)
