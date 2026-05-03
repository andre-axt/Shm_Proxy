CC = gcc
CFLAGS = -Wall -Wextra -g -pthread

LDLIBS = -pthread

TARGET = proxy_server

SRCDIR = src
CACHEDIR = $(SRCDIR)/cache
PROXYDIR = $(SRCDIR)/proxy
SERVERDIR = $(SRCDIR)/server
BUILDDIR = build
BINDIR = bin

SOURCES = $(wildcard $(SRCDIR)/*.c) \
          $(wildcard $(CACHEDIR)/*.c) \
          $(wildcard $(PROXYDIR)/*.c) \
          $(wildcard $(SERVERDIR)/*.c)

OBJECTS = $(patsubst %.c,$(BUILDDIR)/%.o,$(notdir $(SOURCES)))

VPATH = $(SRCDIR) $(CACHEDIR) $(PROXYDIR) $(SERVERDIR)

all: $(BINDIR)/$(TARGET)

$(BINDIR)/$(TARGET): $(OBJECTS) | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)
	@echo "Server compiled successfully at $(BINDIR)/$(TARGET)"
	@echo "To run: ./$(BINDIR)/$(TARGET)"

$(BUILDDIR)/%.o: %.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled: $<"

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

clean:
	rm -rf $(BUILDDIR) $(BINDIR)
	@echo "Cleanup completed"

distclean: clean
	@echo "Deep cleanup completed"

run: $(BINDIR)/$(TARGET)
	./$(BINDIR)/$(TARGET)

rebuild: clean all

debug: CFLAGS += -DDEBUG
debug: clean all

.PHONY: all clean distclean run rebuild debug
