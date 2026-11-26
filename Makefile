#Makefile for lpsk31

SHELL   = /bin/sh
PROJECT = lpsk31
BINDIR  = /usr/local/bin
DOCDIR  = /usr/local/share/doc/
CC       = gcc
CPPFLAGS = -D_FORTIFY_SOURCE=2
CFLAGS   = -Wall -march=native -O2 $(shell pkg-config --cflags hamlib)
LDLIBS   = -lform -lncurses -lasound -lm $(shell pkg-config --libs hamlib)

XDG_CONFIG_HOME ?= $(HOME)/.config
CONFIGDIR      ?= $(XDG_CONFIG_HOME)/$(PROJECT)

objects = main.o phase.o pskcodec.o rig.o screen.o sound.o

$(PROJECT) : $(objects)
	$(CC) $(LDFLAGS) -o $@ $(objects) $(LDLIBS)

$(objects) : common.h
main.o : main.h
phase.o : phase.h
pskcodec.o : pskcodec.h
rig.o : rig.h
screen.o : screen.h
sound.o : sound.h

install : $(PROJECT)
	install -m 755 --strip $(PROJECT) $(BINDIR)
	install -m 644 doc/$(PROJECT).html $(DOCDIR)

# Install a default config file to the user's XDG_CONFIG_HOME (best-effort)
install-config :
	install -d -m 755 $(CONFIGDIR)
	test -e $(CONFIGDIR)/$(PROJECT)rc || \
		install -m 644 default/$(PROJECT)rc $(CONFIGDIR)/$(PROJECT)rc

uninstall :
	rm -f $(BINDIR)/$(PROJECT)
	rm -f $(DOCDIR)/$(PROJECT).html

clean :
	rm -f *.o *~

distclean : clean
	rm -f $(PROJECT)

.PHONY : install install-config uninstall clean distclean
