#!/usr/bin/make
TRGTS = client

CC = g++
RM = rm

BUILDDIR = ../../bin

CFLAGS  = -std=c++11 -Wall -Wextra -Wpedantic -O3
LDFLAGS = 

PRGS = $(addprefix $(BUILDDIR)/, $(TRGTS))
SRCS = $(shell find ./ -name '*.cpp')
OBJS = $(addprefix $(BUILDDIR)/, $(SRCS:.cpp=.o))

all: $(PRGS)

$(BUILDDIR)/client: $(addprefix $(BUILDDIR)/, $(addsuffix .o, client))
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILDDIR)/%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@
