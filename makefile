FLAGS = -Wall -Werror -pthread

all:
	gcc main.c -o main.o ${FLAGS}

clean:
	rm -rf *.o
	rm -rf output.txt