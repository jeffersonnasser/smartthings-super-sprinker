# Your Arduino environment.
AVR_HOME = $(ARD_HOME)/hardware/tools/avr
ARD_BIN = $(AVR_HOME)/bin
AVRDUDE = $(ARD_BIN)/avrdude
AVRDUDE_CONF = $(AVR_HOME)/etc/avrdude.conf

# Your favorite serial monitor.
MON_CMD = screen
MON_SPEED = 9600

# Board settings.
BOARD = uno
VARIANT = uno
PORT = /dev/tty.usbmodem6221
# PROGRAMMER = stk500v2
PROGRAMMER = arduino
# PROGRAMMER = avrispmkii

# Where to find header files and libraries.
INC_DIRS = ./inc
LIB_DIRS = $(addprefix $(ARD_HOME)/libraries/, $(LIBS))
LIBS =

include ../Makefile.master

