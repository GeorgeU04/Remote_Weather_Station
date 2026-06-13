database.out: Build/database.o Build/main.o Build/misc.o
	gcc -o database.out Build/database.o Build/misc.o Build/main.o
Build/main.o: Src/main.c
	gcc -o Build/main.o -c Src/main.c
Build/database.o: Src/database.c
	gcc -o Build/database.o -c Src/database.c
Build/misc.o: Src/misc.c 
	gcc -o Build/misc.o -c Src/misc.c
