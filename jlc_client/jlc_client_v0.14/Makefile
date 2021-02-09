CC=gcc
#CFLAGS=-Iinclude -O3 -Wall
CFLAGS=-Iinclude -O0 -Wall -g

ODIR=obj
LDIR =../lib

LIBS=-lm
ARCH=`arch`

_OBJ = console.o parse_commandline.o jcp_client.o jlc_client_linux.o jlc_udp.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

jlc_client_linux: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 

