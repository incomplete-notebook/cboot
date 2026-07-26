# CBoot - C Project Bootstrapping Tool
# Makefile

CC       = clang-22
CFLAGS   = -Wall -Wextra -g -O0 -Iinclude
LDFLAGS  =
TARGET   = cboot

SRCDIR   = src
OBJDIR   = build
INCDIR   = include

SRCS     = $(SRCDIR)/commands.c \
           $(SRCDIR)/docgen.c \
           $(SRCDIR)/domain.c \
           $(SRCDIR)/generator.c \
           $(SRCDIR)/main.c \
           $(SRCDIR)/parser.c \
           $(SRCDIR)/typecheck.c \
           $(SRCDIR)/utils.c
OBJS     = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))

.PHONY: all clean run test

all: $(TARGET)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(INCDIR)/cboot.h $(INCDIR)/domain.h $(INCDIR)/typecheck.h | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

clean:
	rm -rf $(OBJDIR) $(TARGET)

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	@echo "Running tests..."
	@cd tests && bash run_tests.sh