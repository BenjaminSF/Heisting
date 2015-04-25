CC = gcc
CFLAGS = -lpthread -lcomedi -lm -Wall
DEPS = mainDriver.h elevDriver.h io.h fifoqueue.h costFunction.h orderManager.h networkModule.h channels.h backupManager.h publicTypes.h
SRC = main.c mainDriver.c elevDriver.c io.c fifoqueue.c costFunction.c orderManager.c networkModule.c backupManager.c
TARGET = test

all: $(TARGET)
OBJ = $(SRC:.c=.o)
$(TARGET): $(OBJ)
	$(CC) $^ -o $@ $(CFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJ)

rebuild:	clean all

.PHONY: all rebuild clean

# clang *.c -Wall -fsanitize=address -g -lcomedi -lpthread -lm
#clang *.c -Wall -fsanitize=thread -g -lcomedi -lpthread -lm
#clang *.c -Wall -fsanitize=memory -g -lcomedi -lpthread -lm
