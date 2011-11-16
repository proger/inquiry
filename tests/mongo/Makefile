#	$SYSREVERSE: Makefile,v 1.1 2011/05/04 08:51:14 proger Exp $

PROG=		mongotest

SRCS=		mongo.cc

OBJS=		${SRCS:S/.cc/.o/g}

MONGO?=		${HOME}/dev/ports/mongo

CXX=		c++

#CXXFLAGS=	-v
CXXFLAGS+=	-g -Wall -Werror

CXXFLAGS+=	-I/usr/local/include/c++/4.2.4				\
		-I/usr/local/include/c++/4.2.4/i386-unknown-openbsd4.8	\
		-I/usr/local/include/c++/4.2.4/backward			\

CXXFLAGS+=	-I/usr/local/include					\
		-I${MONGO}						\
		-DMONGO_EXPOSE_MACROS					\
		-I${.CURDIR}

#LDFLAGS?=	-Xlinker -t
LDFLAGS?=	-Xl -t
LDADD?=		

LDFLAGS+=	-L/usr/local/lib					\
		-L${MONGO} 						

LDADD+=		-lstdc++ -lpthread					\
		-lboost_system-mt -lboost_thread-mt			\
		-lboost_filesystem-mt -lboost_program_options-mt	\
		-lpcrecpp -lpcre -lexecinfo				\
		-lmongoclient

${PROG}: ${OBJS}
	${CXX} -o $@ ${OBJS} ${LDFLAGS} ${LDADD}

.SUFFIXES:	.cc .o
.cc.o: ${.PREFIX}.cc
	${CXX} -c ${CXXFLAGS} $< -o $@

obj:
	[ ! -d ${.CURDIR}/obj ] && mkdir -p obj

clean:
	rm -f ${PROG} *.o *.core

.PHONY: clean
