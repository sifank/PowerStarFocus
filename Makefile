#***************************************************************
#  Program:      Makefile
#  Version:      20210103
#  Author:       Sifan S. Kahale
#  Description:  INDI PowerStar Focus
#***************************************************************

CFLAGS = -O2 -Wall -lrt
CC = g++ 

all: hid control tui psfocus

hid:
	cc -Wall -g -fpic -c -Ihidapi `pkg-config libusb-1.0 --cflags` hid.c -o hid.o

control:
	$(CC) $(CFLAGS)  -g -fpic -c -Ihidapi `pkg-config libusb-1.0 --cflags` PScontrol.cpp -o PScontrol.o

support:
	$(CC) $(CFLAGS) -g -fpic -c

tui: hid control
	$(CC) $(CFLAGS) -g -fpic -c  PStui.cpp -o PStui.o
	g++ -Wall -g hid.o PScontrol.o PStui.o `pkg-config libusb-1.0 --libs` -lrt -lpthread -o pstui

psfocus: hid control
	$(CC) $(CFLAGS)  -std=c++11 -I/usr/include -I/usr/include/libindi -c PSfocus.cpp
	$(CC) $(CFLAGS) -std=c++11 -rdynamic hid.o PScontrol.o PSfocus.o  `pkg-config libusb-1.0 --libs` -lpthread -o indi_powerstarfocus -lindidriver
	
clean:
	@rm -rf *.o indi_powerstarfocus pstui

install:
	\cp -f indi_powerstarfocus /usr/bin/
	\cp -f indi_powerstarfocus.xml /usr/share/indi/
	\cp -f pstui /usr/bin/

