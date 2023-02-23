#!/usr/bin/make
TRGTS = pgdb

CC = g++
RM = rm

ROOTDIR = ../..
BUILDDIR = $(ROOTDIR)/bin
INCLUDEDIR = $(ROOTDIR)/include

CFLAGS  = -std=c++17 -Wall -Wextra -Wpedantic -Werror -I./ -I$(INCLUDEDIR) -O3
LDFLAGS =

GLIB = $(shell pkg-config --cflags --libs glib-2.0)
GTKMM = $(shell pkg-config --cflags --libs gtkmm-3.0)
GTKSRCVIEW = $(shell pkg-config --cflags --libs gtksourceviewmm-3.0)
LIBSSH = $(shell pkg-config --cflags --libs libssh)

PRGS = $(addprefix $(BUILDDIR)/, $(TRGTS))
SRCS = $(shell find ./ -name '*.cpp')
OBJS = $(addprefix $(BUILDDIR)/, $(SRCS:.cpp=.o))

all: $(PRGS)

$(BUILDDIR)/libmigdb.a:
	+$(MAKE) -C $(INCLUDEDIR)/libmigdb

$(BUILDDIR)/resources.c:
	cd $(ROOTDIR) && glib-compile-resources pgdb.gresource.xml --target=bin/resources.c --generate-source

$(BUILDDIR)/pgdb: $(addprefix $(BUILDDIR)/, $(addsuffix .o, startup breakpoint breakpoint_dialog canvas window master resources)) $(BUILDDIR)/libmigdb.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(GTKMM) $(GTKSRCVIEW) $(LIBSSH)

$(BUILDDIR)/%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@ $(GTKMM)

$(BUILDDIR)/window.o: window.cpp
	$(CC) $(CFLAGS) -c $< -o $@ $(GTKMM) $(GTKSRCVIEW)

.PHONY: resources
resources: $(BUILDDIR)/resources.o

$(BUILDDIR)/resources.o: $(BUILDDIR)/resources.c
	$(CC) $(CFLAGS) -c $< -o $@ $(GLIB)
