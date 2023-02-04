#!/usr/bin/make
TRGTS = pgdbslave

CC = g++
RM = rm

BUILDDIR = ../../bin

CFLAGS  = -std=c++17 -Wall -Wextra -Wpedantic -Werror -O3
LDFLAGS = 

PRGS = $(addprefix $(BUILDDIR)/, $(TRGTS))
SRCS = $(shell find ./ -name '*.cpp')
OBJS = $(addprefix $(BUILDDIR)/, $(SRCS:.cpp=.o))

all: $(PRGS)

$(BUILDDIR)/pgdbslave: $(addprefix $(BUILDDIR)/, $(addsuffix .o, slave))
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILDDIR)/%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@
