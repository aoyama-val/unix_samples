BIN		= a.out
CC		= gcc
CFLAGS	= -g3 -O0 -W -Wall -Wno-unused-parameter
LDFLAGS	=



# ソースファイル
SRC 	= $(wildcard *.c)
OBJ		= $(SRC:%.c=%.o)

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	@$(BIN)

debug: all
	gdb $(BIN)

clean:
	rm -rf *.o $(BIN)
