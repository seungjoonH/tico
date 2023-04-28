all: tico run

tico: tico.c
	gcc tico.c -o tico -lm

run:
	./tico

clean:
	rm -f tico
