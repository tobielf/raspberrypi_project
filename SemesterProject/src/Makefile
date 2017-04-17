ifneq ($V,1)
Q ?= @
endif

DEBUG	= -O3
CC	= gcc
INCLUDE	= -I/usr/local/include
CFLAGS	= $(DEBUG) -Wall $(INCLUDE) -Winline -pipe

LDFLAGS	= -L/usr/local/lib
LDLIBS    = -levent -lwiringPi -lwiringPiDev -lpthread -lm

SRC = smarthomed.c \
	  screen.c \
	  web_server.c \
	  i2c/i2c_lib.c \
	  i2c/i2c_lcd1620.c \
	  i2c/i2c_bmp180.c \
	  spi/spi_mcp3208.c \
	  pin/pin_motor.c \
	  pin/pin_dht_11.c

OBJ	=	$(SRC:.c=.o)

BINS	=	$(SRC:.c=)

smarthomed: $(OBJ)
	$Q echo [link]
	$Q $(CC) -o $@ $(OBJ) $(LDFLAGS) $(LDLIBS)

.c.o:
	$Q echo [CC] $<
	$Q $(CC) -c $(CFLAGS) $< -o $@

clean:
	$Q echo "[Clean]"
	$Q rm -f $(OBJ) *~ core tags $(BINS)

tags:	$(SRC)
	$Q echo [ctags]
	$Q ctags $(SRC)

depend:
	makedepend -Y $(SRC)
