FLAGS = -Wall -Werror -lpthread

all:producer consumer

producer:
	gcc producer.c -o producer -Wall -Werror
consumer:
	gcc consumer.c -o consumer ${FLAGS}

clean:
	rm producer
	rm consumer
	rm -rf output.txt