CC=gcc
RM=rm -f
CFLAGS=-g -Wall -O1

DEPS=util.h internal_commands.h pipes.h
OBJ=main.o pipes.o util.o internal_commands.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all: minishell clean

minishell: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
	
clean:
	$(RM) $(OBJ)
