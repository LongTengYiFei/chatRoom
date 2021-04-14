all:
	g++ server.cpp -o server.out
	gcc client.c -o client.out

clean:
	rm client.out
	rm server.out
