all: encrypt encrypt-driver.o encrypt-module.o circ-buffer.o clean

encrypt: encrypt-driver.o encrypt-module.o circ-buffer.o
	gcc encrypt-driver.o encrypt-module.o circ-buffer.o -o encrypt

encrypt-driver.o: encrypt-driver.c encrypt-module.h circ-buffer.h
	gcc -c encrypt-driver.c -lpthread -lrt

encrypt-module.o: encrypt-module.c encrypt-module.h
	gcc -c encrypt-module.c -lpthread -lrt

circ-buffer.o: circ-buffer.c circ-buffer.h
	gcc -c circ-buffer.c

clean:
	rm -f *.o *~