#Makefile for lpsk31

SHELL   = /bin/sh
PROJECT = lpsk31
BINDIR  = /usr/local/bin
DOCDIR  = /usr/local/share/
CC      = gcc -Wall -O2 -march=native

objects = cat.o lpsk31.o phase.o pskcodec.o screen.o sound.o

$(PROJECT) : $(objects)
	$(CC) -lform -lncurses -o $(PROJECT) $(objects)

$(objects) : $(PROJECT).h

install : $(PROJECT)
	install -m 755 --strip $(PROJECT) $(BINDIR)
	install -m 644 doc/$(PROJECT).html $(DOCDIR)
	if test ! -d $(HOME)/$(PROJECT); then mkdir $(HOME)/$(PROJECT); fi
	install -m 644 default/$(PROJECT)rc $(HOME)/$(PROJECT)

uninstall :
	if test -d $(HOME)/$(PROJECT); then rm -r $(HOME)/$(PROJECT); fi
	rm $(BINDIR)/$(PROJECT)
	rm $(DOCDIR)/$(PROJECT).html

clean :
	rm -f *.o *~

distclean : clean
	rm -f $(PROJECT)
