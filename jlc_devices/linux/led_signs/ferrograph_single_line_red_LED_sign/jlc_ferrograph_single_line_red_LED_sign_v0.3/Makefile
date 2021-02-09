# Raspberry pi specific code. Only build on that machine !

CC=gcc
CFLAGS=-Iinclude -Ofast -Wall -g -DRPI

ODIR=obj
LDIR =../lib

LIBS=-lm -lrt -lpthread
ARCH=`arch`

_OBJ = jlc_udp.o jcp-client-linux.o jcp_client.o jlc_ferrograph_led_sign.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

jlc_ferrograph_led_sign: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 

