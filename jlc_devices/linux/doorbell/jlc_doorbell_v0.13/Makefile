CC=gcc
#CFLAGS=-Iinclude -O3 -Wall
CFLAGS=-Iinclude -O0 -Wall -g -DUSE_ALSA

ODIR=obj
LDIR =../lib

LIBS=-lm -lasound
ARCH=`arch`

_OBJ = jcp_client.o jlc_udp.o wavplay.o jlc_doorbell.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

jlc_doorbell: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 

