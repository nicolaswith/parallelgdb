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

resources:
	+$(MAKE) -C $(SRCDIR)/master resources
	
install:
	cp ./bin/pgdb $(INSTALLDIR)
	cp ./bin/pgdbslave $(INSTALLDIR)
	cp ./res/breakpoint.svg /usr/share/icons/hicolor/scalable/apps/pgdb.svg
	cp ./res/pgdb.desktop /usr/share/applications

uninstall:
	rm -f $(INSTALLDIR)/pgdb
	rm -f $(INSTALLDIR)/pgdbslave
	rm -f /usr/share/icons/hicolor/scalable/apps/pgdb.svg
	rm -f /usr/share/applications/pgdb.desktop

clean:
	$(RM) -f $(BUILDDIR)/*
