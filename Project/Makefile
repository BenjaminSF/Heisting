CC = gcc
CFLAGS = -lpthread -lm -lcomedi
DEPS = mainDriver.h elevDriver.h io.h fifoqueue.h costFunction.h orderManager.h network_modulev2.h channels.h
SRC = main.c mainDriver.c elevDriver.c io.c fifoqueue.c costFunction.c orderManager.c network_modulev2.c
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
