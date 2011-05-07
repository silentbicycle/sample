COPTS= -Wall -O3
#COPTS+= -g

sample: sample.c Makefile
	${CC} -o sample sample.c ${COPTS}

clean:
	rm -f *.core sample
