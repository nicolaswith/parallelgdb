#!/usr/bin/make

BUILDDIR = ./bin
INCLUDEDIR = ./include
SRCDIR = ./src
INSTALLDIR = /usr/local/bin

all: libmigdb master slave

libmigdb:
	+$(MAKE) -C $(INCLUDEDIR)/libmigdb

master:
	+$(MAKE) -C $(SRCDIR)/master

slave:
	+$(MAKE) -C $(SRCDIR)/slave
	
install:
	cp ./bin/pgdbmaster $(INSTALLDIR)
	cp ./bin/pgdbslave $(INSTALLDIR)

uninstall:
	rm -f $(INSTALLDIR)/pgdbmaster
	rm -f $(INSTALLDIR)/pgdbslave

clean:
	$(RM) -f $(BUILDDIR)/*
