PROJECT =	sample
OPTIMIZE =	-O3
WARN =		-Wall -pedantic #-Wextra
#CDEFS +=
#CINCS +=
CSTD +=		-std=c99 #-D_POSIX_C_SOURCE=200112L -D_C99_SOURCE

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

install:
	${INSTALL} -c ${PROJECT} ${PREFIX}/bin

uninstall:
	${RM} -f ${PREFIX}/bin/${PROJECT}
