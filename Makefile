#!/usr/bin/make

BUILDDIR = ./bin
SRCDIR = ./src

all:
	+$(MAKE) -C $(SRCDIR)/server
	+$(MAKE) -C $(SRCDIR)/client

clean:
	$(RM) -f $(BUILDDIR)/*
