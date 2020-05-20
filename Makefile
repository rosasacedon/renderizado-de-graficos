nodos := 16
run := mpirun
mcc  := mpicc
cc := gcc

all: 
	$(mcc) pract2.c -o pract2 -lX11

Blue:
	$(run) -np 1 ./pract2 B
Green:
	$(run) -np 1 ./pract2 G
Red:
	$(run) -np 1 ./pract2 R
BlackWhite:
	$(run) -np 1 ./pract2 W
Sepia:
	$(run) -np 1 ./pract2 S
Negative:
	$(run) -np 1 ./pract2 N
Cianotipia sobre blanco:
	$(run) -np 1 ./pract2 C
Cianotipia sobre negro:
	$(run) -np 1 ./pract2 E
Cianotipia sobre rojo:
	$(run) -np 1 ./pract2 O

run:
	$(run) -np 1 ./pract2 A
