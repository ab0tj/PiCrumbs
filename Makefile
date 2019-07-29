C=g++
CFLAGS=-c -Wall
PC_OBJS=beacon.o console.o gps.o hamlib.o http.o init.o main.o gpio.o predict.o psk.o stringfuncs.o tnc.o INIReader.o varicode.o ini.o sensor.o
PSK_OBJS=psk.o pskgen.o varicode.o

all: picrumbs

install: all
	/usr/bin/install -D --mode=755 picrumbs /usr/local/bin/picrumbs

picrumbs: $(PC_OBJS)
	$(C) $(PC_OBJS) -lpthread -lhamlib -lhamlib++ -lcurl -lgps -o picrumbs

pskgen: $(PSK_OBJS)
	$(C) $(PSK_OBJS) -o pskgen

beacon.o: beacon.cpp version.h
	$(C) $(CFLAGS) -o beacon.o beacon.cpp

console.o: console.cpp version.h
	$(C) $(CFLAGS) -o console.o console.cpp

gps.o: gps.cpp
	$(C) $(CFLAGS) -o gps.o gps.cpp

hamlib.o: hamlib.cpp
	$(C) $(CFLAGS) -o hamlib.o hamlib.cpp

http.o: http.cpp version.h
	$(C) $(CFLAGS) -o http.o http.cpp

init.o: init.cpp version.h
	$(C) $(CFLAGS) -o init.o init.cpp

main.o: main.cpp
	$(C) $(CFLAGS) -o main.o main.cpp

gpio.o: gpio.cpp
	$(C) $(CFLAGS) -o gpio.o gpio.cpp

predict.o: predict.cpp
	$(C) $(CFLAGS) -o predict.o predict.cpp

stringfuncs.o: stringfuncs.cpp
	$(C) $(CFLAGS) -o stringfuncs.o stringfuncs.cpp

tnc.o: tnc.cpp
	$(C) $(CFLAGS) -o tnc.o tnc.cpp

INIReader.o: INIReader.cpp
	$(C) $(CFLAGS) -o INIReader.o INIReader.cpp

ini.o: ini.c
	$(C) $(CFLAGS) -o ini.o ini.c
	
psk.o: psk.cpp
	$(C) $(CFLAGS) -o psk.o psk.cpp
	
pskgen.o: pskgen.cpp
	$(C) $(CFLAGS) -o pskgen.o pskgen.cpp
	
varicode.o: varicode.cpp
	$(C) $(CFLAGS) -o varicode.o varicode.cpp

sensor.o: sensor.cpp
	$(C) $(CFLAGS) -o sensor.o sensor.cpp

clean:
	rm -f picrumbs pskgen $(PC_OBJS) $(PSK_OBJS)
