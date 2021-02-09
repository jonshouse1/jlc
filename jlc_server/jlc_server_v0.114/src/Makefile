CC=gcc
#CFLAGS=-Iinclude -O3 -Wall
CFLAGS=-Iinclude -O0 -Wall -g

ODIR=obj
LDIR =../lib

LIBS=-lm
ARCH=`arch`

_OBJ = jlc_log.o jlc_cmdline.o jlc_isdaylight.o jlc_http.o jlc_group.o jlp.o jlc.o jlc_monitor.o jlc_time.o jlc_devices.o jlc_dev_create_devfiles.o jlc_console.o jlc_tcpconn.o jlc_udp.o jlc_variable_subs.o jlc_interpreter.o jlc_runscript.o jlc_universe.o jlc_effects.o jlc_mask.o jcp.o jlc_audioplus.o jlc_sound_sequencer.o jlc_binmsg.o jlc_colour_presets.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

jlcd: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)
	mv jlcd ..

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 

