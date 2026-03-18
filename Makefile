CXX = clang
LDFLAGS = -lm -lraylib
CFLAGS = -std=gnu17 -Wall -Wextra -O2
FILES = $(wildcard src/*.c)
OBJS = $(FILES:.c=.o)
TARGET = bin/bezier

.PHONY: all clean
all: $(TARGET)
$(TARGET): $(OBJS)
	$(CXX) $(CFLAGS) -o $@ $^ $(LDFLAGS)
%.o: %.c
	$(CXX) $(CFLAGS) -c $< -o $@
clean:
	rm -f $(OBJS) $(TARGET)
