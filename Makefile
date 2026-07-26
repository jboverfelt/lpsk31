#Makefile for lpsk31

SHELL   = /bin/sh
PROJECT = lpsk31
BINDIR  = /usr/local/bin
DOCDIR  = /usr/local/share/doc/
CC       = gcc
CPPFLAGS = -D_FORTIFY_SOURCE=2
CFLAGS   = -Wall -march=native -O2
LDLIBS   = -lform -lncurses -lasound -lm

objects = cat.o main.o phase.o pskcodec.o screen.o sound.o

$(PROJECT) : $(objects)
	$(CC) $(LDFLAGS) -o $@ $(objects) $(LDLIBS)

$(objects) : common.h
cat.o : cat.h
main.o : main.h
phase.o : phase.h
pskcodec.o : pskcodec.h
screen.o : screen.h
sound.o : sound.h

install : $(PROJECT)
	install -m 755 --strip $(PROJECT) $(BINDIR)
	install -m 644 doc/$(PROJECT).html $(DOCDIR)
	$(MAKE) install-config

install-config :
	install -d -m 755 $(HOME)/$(PROJECT)
	test -e $(HOME)/$(PROJECT)/$(PROJECT)rc || \
		install -m 644 default/$(PROJECT)rc $(HOME)/$(PROJECT)/$(PROJECT)rc

uninstall :
	if test -d $(HOME)/$(PROJECT); then rm -r $(HOME)/$(PROJECT); fi
	rm $(BINDIR)/$(PROJECT)
	rm $(DOCDIR)/$(PROJECT).html

clean :
	rm -f *.o *~

distclean : clean
	rm -f $(PROJECT)
