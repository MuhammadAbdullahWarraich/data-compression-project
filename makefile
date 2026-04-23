#DEBUG_FLAG = -g -Wall -Wextra -Werror -fsanitize=undefined,address -fno-strict-aliasing
#DEBUG_FLAG = -g -Wall -Wextra -Werror
#CFLAGS = -std=c23 $(DEBUG_FLAG) -O3
CFLAGS = -std=c23 -O3

all: main.o ini.o rle.o
	gcc $(CFLAGS) main.o ini.o rle.o -o main
	rm *.o

main.o: main.c
	gcc $(CFLAGS) -c main.c

ini.o: ./third-party/inih/ini.c
	gcc $(CFLAGS) -DINI_ALLOW_INLINE_COMMENTS=1 -DINI_INLINE_COMMENT_PREFIXES="\"#\"" -c ./third-party/inih/ini.c

*.o: *.c
	gcc $(CFLAGS) -c *.c

clean:
	rm main
