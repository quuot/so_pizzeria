
all: mainp client manager firefighter

mainp: mainp.c utils.c
	gcc mainp.c utils.c -o bin/mainp

client: client.c utils.c
	gcc client.c utils.c -o bin/client

manager: manager.c utils.c
	gcc manager.c utils.c -o bin/manager

firefighter: firefighter.c utils.c
	gcc firefighter.c utils.c -o bin/firefighter

clean:
	rm -f bin/mainp bin/client bin/manager bin/firefighter
