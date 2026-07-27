# CBoot - C Project Bootstrapping Tool
# Makefile

CC       = gcc
CFLAGS   = -Wall -Wextra -g -O0 -Isrc
LDFLAGS  =
TARGET   = cboot

SRCDIR   = src
OBJDIR   = build

SRCS     = $(SRCDIR)/main.c \
           $(SRCDIR)/domain/domain.c \
           $(SRCDIR)/commands/commands.c \
           $(SRCDIR)/parser/parser.c \
           $(SRCDIR)/generator/generator.c \
           $(SRCDIR)/docgen/docgen.c \
           $(SRCDIR)/typecheck/typecheck.c \
           $(SRCDIR)/utils/utils.c

OBJS     = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))

.PHONY: all clean run test

all: $(TARGET)

$(OBJDIR):
	mkdir -p $(OBJDIR)/domain
	mkdir -p $(OBJDIR)/commands
	mkdir -p $(OBJDIR)/parser
	mkdir -p $(OBJDIR)/generator
	mkdir -p $(OBJDIR)/docgen
	mkdir -p $(OBJDIR)/typecheck
	mkdir -p $(OBJDIR)/utils

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(SRCDIR)/cboot.h | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

clean:
	rm -rf $(OBJDIR) $(TARGET)

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	@echo "Running tests..."
