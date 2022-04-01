.PHONY: cstorage cstorage-client lint test test-docker

SRCS = arguments.c connection.c epoll.c handler_get.c handlers.c handler_set.c main.c socket.c
OBJS = ${SRCS:.c=.o}

CC = clang-12
FLAGS = -std=c2x -fsanitize=address,undefined -fno-sanitize-recover=all -O0 -g -Wall -Wextra -pedantic

CLIENT_FLAGS = -std=gnu2x -fsanitize=address,undefined -fno-sanitize-recover=all -O0 -g -Wall -Wextra -pedantic

all: cstorage cstorage-client

cstorage: ${OBJS}
	${CC} ${FLAGS} ${OBJS} -o $@

cstorage-client:
	${CC} ${CLIENT_FLAGS} client.c -o $@

.c.o:
	${CC} ${FLAGS} -c $< -o $@

clean:
	rm *.o

lint:
	clang-tidy ./*.c

test: cstorage cstorage-client
	cd test && ./test.sh

test-docker:
	docker build --tag test-image --file ./test/docker/Dockerfile . 
	docker run test-image

