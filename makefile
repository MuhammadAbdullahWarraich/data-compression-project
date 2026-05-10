#DEBUG_FLAG = -g -Wall -Wextra -Werror -fsanitize=undefined,address -fno-strict-aliasing
#DEBUG_FLAG = -g -Wall -Wextra -Werror
#CFLAGS = -std=c23 $(DEBUG_FLAG) -O3
CFLAGS = -std=c2x -O3 -Iinclude

all: main

main: build/main.o build/bwt.o build/rle.o build/ini.o
	gcc $(CFLAGS) $^ -o $@

build/main.o: main.c | build
	gcc $(CFLAGS) -c $< -o $@

build/bwt.o: bwt.c | build
	gcc $(CFLAGS) -c $< -o $@

build/rle.o: rle.c | build
	gcc $(CFLAGS) -c $< -o $@

build/ini.o: third-party/inih/ini.c | build
	gcc $(CFLAGS) -DINI_ALLOW_INLINE_COMMENTS=1 -DINI_INLINE_COMMENT_PREFIXES='"#"' -c $< -o $@

build:
	mkdir -p $@

clean:
	rm -rf main build
