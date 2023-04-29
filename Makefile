all: tico run

tico: tico.c
	gcc tico.c -o tico -lm

run: tico $(file)
	./tico $(file)

clean:
	rm -f tico
