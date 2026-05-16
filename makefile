#DEBUG_FLAG = -g -Wall -Wextra -Werror -fsanitize=undefined,address -fno-strict-aliasing
#DEBUG_FLAG = -g -Wall -Wextra -Werror
#CFLAGS = -std=c23 $(DEBUG_FLAG) -O3
CFLAGS = -std=c2x -O3 -Iinclude -D_POSIX_C_SOURCE=200809L

PROG = main

OBJS = build/main.o \
       build/block.o \
       build/bwt.o \
       build/rle.o \
       build/rle2.o \
       build/mtf.o \
       build/huffman.o \
       build/compressor.o \
       build/ini.o

all: $(PROG)

$(PROG): $(OBJS)
	gcc $(CFLAGS) $^ -o $@

build/main.o: main.c | build
	gcc $(CFLAGS) -c $< -o $@

build/block.o: src/block.c | build
	gcc $(CFLAGS) -c $< -o $@

build/bwt.o: src/bwt.c | build
	gcc $(CFLAGS) -c $< -o $@

build/rle.o: src/rle.c | build
	gcc $(CFLAGS) -c $< -o $@

build/rle2.o: src/rle2.c | build
	gcc $(CFLAGS) -c $< -o $@

build/mtf.o: src/mtf.c | build
	gcc $(CFLAGS) -c $< -o $@

build/huffman.o: src/huffman.c | build
	gcc $(CFLAGS) -c $< -o $@

build/compressor.o: src/compressor.c | build
	gcc $(CFLAGS) -c $< -o $@

build/ini.o: third-party/inih/ini.c | build
	gcc $(CFLAGS) -DINI_ALLOW_INLINE_COMMENTS=1 -DINI_INLINE_COMMENT_PREFIXES='"#"' -c $< -o $@

build:
	mkdir -p $@

test: $(PROG)
	bash scripts/test_roundtrip.sh

benchmark: $(PROG)
	bash scripts/benchmark.sh

clean:
	rm -rf $(PROG) build
