# Raspberry pi (v3) specific code. Only build on that machine !

CC=gcc
CFLAGS=-Iinclude -Ofast -Wall -g -DRPI

ODIR=obj
LDIR =../lib

LIBS=-lm -lpigpio -lrt -lpthread
ARCH=`arch`

_OBJ = jlc_udp.o jcp-client-linux.o jcp_client.o jlc_udp.o jlc_6_line_led_sign_RPI.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

jlc_6_line_led_sign_RPI: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 

