CC=gcc
parsing: parsing.c
	$(CC) parsing.c mpc.c -o parsing -ledit -lm -Wall -Wextra -pedantic -std=c99
