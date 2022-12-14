#!/usr/bin/make
TRGTS = server

CC = g++
RM = rm

BUILDDIR = ../../bin
INCLUDEDIR = ../../include

CFLAGS  = -std=c++17 -Wall -Wextra -Wpedantic -I./ -I$(INCLUDEDIR) -O3
LDFLAGS = -lpthread $(BUILDDIR)/libmigdb.a
GTKLIBS = $(shell pkg-config --cflags --libs gtkmm-3.0 gtksourceviewmm-3.0 libssh)

PRGS = $(addprefix $(BUILDDIR)/, $(TRGTS))
SRCS = $(shell find ./ -name '*.cpp')
OBJS = $(addprefix $(BUILDDIR)/, $(SRCS:.cpp=.o))

all: $(PRGS)

$(BUILDDIR)/server: $(addprefix $(BUILDDIR)/, $(addsuffix .o, startup breakpoint breakpoint_dialog canvas window server))
	+$(MAKE) -C $(INCLUDEDIR)/libmigdb
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(GTKLIBS)

$(BUILDDIR)/%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@ $(GTKLIBS)
