SRC=$(wildcard *.c)
DST=$(patsubst %.c,%,$(SRC))

default: $(DST)

$(DST): $(SRC) usys.S
	gcc -o $@ $^

clean:
	rm -rf $(DST)
