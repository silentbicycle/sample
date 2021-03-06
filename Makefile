PROJECT =	sample
OPTIMIZE =	-O3
WARN =		-Wall -pedantic
#CDEFS +=
#CINCS +=
CSTD +=		-std=c99 -D_BSD_SOURCE #-D_POSIX_C_SOURCE=200112L

CFLAGS +=	${CSTD} -g ${WARN} ${CDEFS} ${CINCS} ${OPTIMIZE}

all: ${PROJECT}

OBJS=	${PROJECT}.o

# Basic targets

${PROJECT}: main.o ${OBJS}
	${CC} -o $@ main.o ${OBJS} ${LDFLAGS}

clean:
	rm -f ${PROJECT} test_${PROJECT} *.o *.a *.core

# Installation
PREFIX ?=	/usr/local
INSTALL ?=	install
RM ?=		rm

install: ${PROJECT}
	${INSTALL} -c ${PROJECT} ${PREFIX}/bin

uninstall:
	${RM} -f ${PREFIX}/bin/${PROJECT}
