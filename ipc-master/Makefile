

default: carrace

carrace: carrace.c
	gcc -O3 -o carrace carrace.c

run: carrace
	./carrace > out 2>&1

clean:
	rm -rf carrace out
