CC = gcc
CFLAGS = -Wall -Wextra

# Define sources and executables
APP_SRC = app.c
SLAVE_SRC = slave.c
HEADERS = include/lib.h
APP = app
SLAVE = slave

# Default target
all: $(APP) $(SLAVE)

# Compile app
$(APP): $(APP_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o $(APP) $(APP_SRC)

# Compile slave
$(SLAVE): $(SLAVE_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o $(SLAVE) $(SLAVE_SRC)

# Clean target
clean:
	rm -f $(APP) $(SLAVE) result.txt

.PHONY: all clean
