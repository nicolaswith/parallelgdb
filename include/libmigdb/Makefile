#!/usr/bin/make

STATICLIB = libmigdb.a

BUILDDIR = ../../bin
INCLUDEDIR = ../

CC = g++
CFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -Werror -O3 -I$(INCLUDEDIR)

SRCS = $(shell find ./ -name '*.c')
OBJS = $(addprefix $(BUILDDIR)/, $(SRCS:.c=.o))

$(BUILDDIR)/$(STATICLIB): $(OBJS)
	ar rcs $@ $^

$(BUILDDIR)/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@