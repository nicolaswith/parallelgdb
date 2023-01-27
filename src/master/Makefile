#!/usr/bin/make
TRGTS = pgdbmaster

CC = g++
RM = rm

BUILDDIR = ../../bin
INCLUDEDIR = ../../include

CFLAGS  = -std=c++17 -Wall -Wextra -Wpedantic -Werror -I./ -I$(INCLUDEDIR) -O3
LDFLAGS =

GTKMM = $(shell pkg-config --cflags --libs gtkmm-3.0)
GTKSRCVIEW = $(shell pkg-config --cflags --libs gtksourceviewmm-3.0)
LIBSSH = $(shell pkg-config --cflags --libs libssh)

PRGS = $(addprefix $(BUILDDIR)/, $(TRGTS))
SRCS = $(shell find ./ -name '*.cpp')
OBJS = $(addprefix $(BUILDDIR)/, $(SRCS:.cpp=.o))

all: $(PRGS)

$(BUILDDIR)/libmigdb.a:
	+$(MAKE) -C $(INCLUDEDIR)/libmigdb

$(BUILDDIR)/pgdbmaster: $(addprefix $(BUILDDIR)/, $(addsuffix .o, startup breakpoint breakpoint_dialog canvas window master)) $(BUILDDIR)/libmigdb.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(GTKMM) $(GTKSRCVIEW) $(LIBSSH)

$(BUILDDIR)/%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@ $(GTKMM)

$(BUILDDIR)/window.o: window.cpp
	$(CC) $(CFLAGS) -c $< -o $@ $(GTKMM) $(GTKSRCVIEW)