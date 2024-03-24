CC = gcc
CFLAGS = -Wall
all: app slave 

app: app.c 
	$(CC) $(CFLAGS) -o app app.c 

slave: slave.c
	$(CC) $(CFLAGS) -o slave slave.c

clean:
	rm -f app slave  
