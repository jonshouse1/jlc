CC=gcc
#CFLAGS=-Iinclude -O3 -Wall
CFLAGS=-Iinclude -O0 -Wall -g -DUSE_ALSA
#CFLAGS=-Iinclude -O0 -Wall -g -DUSE_ALSA -DWITHTERM

ODIR=obj
LDIR =../lib

LIBS=-lm -lforms -lasound -lX11
ARCH=`arch`

_OBJ = jlc_linuxgui.o jlc_binmsg.o jlc_linuxgui_tcp.o jcp_discover.o wavplay.o parse_commandline.o jlc_gui.o jlc_gui_xforms.o jlc_gui_xforms_form_control.o jlc_gui_xforms_form_devices.o jlc_gui_xforms_form_scripts.o jlc_gui_xforms_form_terminal.o jlc_gui_xforms_form_colorselector.o get_x11_colour_under_cursor.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) -DLINUX -DGUIXFORMS

jlc_linuxgui_bin_xforms: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS) -DLINUX -DGUIXFORMS

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 

