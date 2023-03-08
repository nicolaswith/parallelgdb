#!/usr/bin/make

RM = rm -f

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
	$(RM) $(INSTALLDIR)/pgdb
	$(RM) $(INSTALLDIR)/pgdbslave
	$(RM) /usr/share/icons/hicolor/scalable/apps/pgdb.svg
	$(RM) /usr/share/applications/pgdb.desktop

clean:
	$(RM) $(BUILDDIR)/*
