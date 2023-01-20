#!/usr/bin/make

BUILDDIR = ./bin
INCLUDEDIR = ./include
SRCDIR = ./src

all: libmigdb master slave

libmigdb:
	+$(MAKE) -C $(INCLUDEDIR)/libmigdb

master:
	+$(MAKE) -C $(SRCDIR)/master

slave:
	+$(MAKE) -C $(SRCDIR)/slave

clean:
	$(RM) -f $(BUILDDIR)/*
