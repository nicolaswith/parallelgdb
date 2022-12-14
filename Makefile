#!/usr/bin/make

BUILDDIR = ./bin
INCLUDEDIR = ./include
SRCDIR = ./src

all: libmigdb server client

libmigdb:
	+$(MAKE) -C $(INCLUDEDIR)/libmigdb

server:
	+$(MAKE) -C $(SRCDIR)/server

client:
	+$(MAKE) -C $(SRCDIR)/client

clean:
	$(RM) -f $(BUILDDIR)/*
