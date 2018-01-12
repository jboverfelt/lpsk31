#Makefile for lpsk31

SHELL   = /bin/sh
PROJECT = lpsk31
BINDIR  = /usr/local/bin
DOCDIR  = /usr/local/share/doc/
CC      = gcc -Wall -march=native -O2 -D_FORTIFY_CODE=2

objects = cat.o main.o phase.o pskcodec.o screen.o sound.o

$(PROJECT) : $(objects)
	$(CC) -lform -lncurses -lasound -lm -o $(PROJECT) $(objects)

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
