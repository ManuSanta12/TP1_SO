CC = gcc
CFLAGS = -Wall -Wextra

# Define sources and executables
APP_SRC = app.c
SLAVE_SRC = slave.c
VIEW_SRC = view.c
HEADERS = include/lib.h
APP = app
SLAVE = slave
VIEW = view

# Default target
all: $(APP) $(SLAVE) $(VIEW)

# Compile app
$(APP): $(APP_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o $(APP) $(APP_SRC)

# Compile slave
$(SLAVE): $(SLAVE_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o $(SLAVE) $(SLAVE_SRC)

# Compile view
$(VIEW): $(VIEW_SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o $(VIEW) $(VIEW_SRC)

# Clean target
clean:
	rm -f $(APP) $(SLAVE) $(VIEW) result.txt

.PHONY: all clean
