DEBUG_FLAG = -g -Wall -Wextra -Werror 
CFLAGS = -std=c23 $(DEBUG_FLAG)

all: main.o ini.o
	gcc $(CFLAGS) main.o ini.o -o main
	rm *.o

main.o: main.c
	gcc $(CFLAGS) -c main.c

ini.o: ./third-party/inih/ini.c
	gcc $(CFLAGS) -DINI_ALLOW_INLINE_COMMENTS=1 -DINI_INLINE_COMMENT_PREFIXES="\"#\"" -c ./third-party/inih/ini.c

bwt.o: bwt.c
	gcc $(CFLAGS) -c bwt.c
clean:
	rm main
