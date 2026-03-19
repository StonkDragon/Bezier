CXX = clang
LDFLAGS = -lm -lraylib
CFLAGS = -std=gnu17 -Wall -Wextra -O2 -Wall -Wextra -Werror -pedantic -Iinclude
FILES = $(wildcard src/*.c)
OBJS = $(FILES:.c=.o)
TARGET = bin/bezier

.PHONY: all clean
all: $(TARGET)
$(TARGET): $(OBJS)
	mkdir -p $(dir $@)
	$(CXX) $(CFLAGS) -o $@ $^ $(LDFLAGS)
%.o: %.c
	$(CXX) $(CFLAGS) -c $< -o $@
clean:
	rm -f $(OBJS) $(TARGET)
