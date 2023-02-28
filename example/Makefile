#!/usr/bin/make
TRGTS = target

CC = mpic++
RM = rm

BUILDDIR = ./bin

CFLAGS  = -std=c++17 -Wall -Wextra -Wpedantic -O0 -g
LDFLAGS = 

PRGS = $(addprefix $(BUILDDIR)/, $(TRGTS))
SRCS = $(shell find ./ -name '*.cpp')
OBJS = $(addprefix $(BUILDDIR)/, $(SRCS:.cpp=.o))

all: $(PRGS)

$(BUILDDIR)/target: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILDDIR)/%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@



clean:
	$(RM) -f $(BUILDDIR)/*
