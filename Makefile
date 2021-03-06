DEVICE = atmega1284p
MCU = atmega1284p
AVRDUDE_DEVICE = m1284p
PORT ?= /dev/ttyACM0
DEVICE ?= atmega168
MCU ?= atmega168
AVRDUDE_DEVICE ?= m168

OPTIMIZE ?= -Os
CF ?= $(OPTIMIZE)
CFLAGS=-g -Wall -mcall-prologues -mmcu=$(MCU) $(DEVICE_SPECIFIC_CFLAGS) $(CF) -DUSE_FLOATS=1 -Werror
CC=avr-gcc
OBJ2HEX=avr-objcopy 
LDFLAGS=-Wl,-gc-sections -lpololu_$(DEVICE) -Wl,-relax -lc

PORT ?= /dev/ttyACM0
AVRDUDE=avrdude

LIB= \
	kcmd.o \
	kdebug.o \
	kio.o \
	klinebuf.o \
	kmotor.o \
	kserial.o \
	ktimers.o \
	kutils.o \
	PDControl.o \
	Trajectory.o \

all: asgn1.hex stop.hex lab1.hex lab2.hex

asgn1:	asgn1.hex
	$(AVRDUDE) -p $(AVRDUDE_DEVICE) -c avrisp2 -P $(PORT) -U flash:w:$?

ASGN1=asgn1.o $(LIB)

asgn1.obj:	$(ASGN1)
	$(CC) $(CFLAGS) $(ASGN1) $(LDFLAGS) -o $@

stop:	stop.hex
	$(AVRDUDE) -p $(AVRDUDE_DEVICE) -c avrisp2 -P $(PORT) -U flash:w:$?

STOP=stop.o $(LIB)

stop.obj:	$(STOP)
	$(CC) $(CFLAGS) $(STOP) $(LDFLAGS) -o $@

lab1:	lab1.hex
	$(AVRDUDE) -p $(AVRDUDE_DEVICE) -c avrisp2 -P $(PORT) -U flash:w:$?

LAB1=lab1.o $(LIB)

lab1.obj:	$(LAB1)
	$(CC) $(CFLAGS) $(LAB1) $(LDFLAGS) -o $@

lab2:	lab2.hex
	$(AVRDUDE) -p $(AVRDUDE_DEVICE) -c avrisp2 -P $(PORT) -U flash:w:$?

LAB2=lab2.o lab2cmds.o $(LIB)

lab2.obj:	$(LAB2)
	$(CC) $(CFLAGS) $(LAB2) $(LDFLAGS) -o $@

clean:
	rm -f *.o *.hex *.obj

%.hex:	%.obj
	$(OBJ2HEX) -R .eeprom -O ihex $< $@
