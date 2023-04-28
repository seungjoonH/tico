all: tico run

tico: tico.c
	gcc tico.c -o tico

run:
	./tico

clean:
	rm -f tico
