all: myk

myk:
	gcc client.c -o BibakBOXClient
	gcc server.c -o BibakBOXServer -pthread

clean:
	rm *.o
	rm *.txt