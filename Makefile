
CC = gcc

all:
	$(CC) -w -omud attack.c command.c  database.c error.c getconfig.c help.c lookup.c monster.c mud.c password.c shutdown.c spell.c string.c  user.c -lcrypt
	$(CC) -c genpass.c -o genpass.o -lcrypt
	$(CC) -o genpass genpass.o -lcrypt

win32:
	$(CC) -w -o mud.exe attack.c error.c password.c shutdown.c spell.c command.c getconfig.c help.c mud.c lookup.c monster.c database.c user.c string.c crypt.c -lws2_32

	$(CC) -c crypt.c genpass.c
	$(CC) -o genpass genpass.o crypt.c
clean:
	rm mud genpass *.o


