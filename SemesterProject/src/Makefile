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

debug: CFLAGS += -DXTEST -DDEBUG -g
debug: unittest

integratedtest: CFLAGS += -DYTEST -DDEBUG -g
integratedtest: component

component: $(OBJ)
	$Q echo [build component]
	mkdir component
	$Q $(CC) -o ./component/screen ./i2c/i2c_lib.o ./i2c/i2c_lcd1620.o ./i2c/i2c_bmp180.o ./pin/pin_dht_11.o ./spi/spi_mcp3208.o ./screen.o $(LDFLAGS) $(LDLIBS)

unittest: $(OBJ)
	$Q echo [build unittest]
	mkdir unittest
	$Q $(CC) -o ./unittest/i2c_lcd1620 ./i2c/i2c_lib.o ./i2c/i2c_lcd1620.o $(LDFLAGS) $(LDLIBS)
	$Q $(CC) -o ./unittest/i2c_bmp180 ./i2c/i2c_lib.o ./i2c/i2c_bmp180.o $(LDFLAGS) $(LDLIBS)
	$Q $(CC) -o ./unittest/spi_mcp3208 ./spi/spi_mcp3208.o $(LDFLAGS) $(LDLIBS)
	$Q $(CC) -o ./unittest/pin_motor ./pin/pin_motor.o $(LDFLAGS) $(LDLIBS)
	$Q $(CC) -o ./unittest/pin_dht_11 ./pin/pin_dht_11.o $(LDFLAGS) $(LDLIBS)

.c.o:
	$Q echo [CC] $<
	$Q $(CC) -c $(CFLAGS) $< -o $@

clean:
	$Q echo "[Clean]"
	$Q rm -f $(OBJ) *~ core tags $(BINS)
	$Q rm -rf unittest/ component/

tags:	$(SRC)
	$Q echo [ctags]
	$Q ctags $(SRC)

depend:
	makedepend -Y $(SRC)
