#	$SYSREVERSE: Makefile,v 1.10 2011/05/15 09:55:32 proger Exp $

INQ=	inq
MONGO=	mongo

TESTS=	acpireg.c anoninitializers.c anonsimple.c anontypedef.c		\
	anonvar.c case01.c expansions.c forward.c forward2.c		\
	inheritance.c initializers.c mbuf.c structs.c sysconf-simple.c	\
	sysconf.c typedefinit.c
OUTS=	${TESTS:S/.c/.out/g}

.SUFFIXES: .c .out

.c.out: $*.c
	@echo "db.$*.drop()" | ${MONGO} --quiet test >/dev/null
	@echo "$@: test.$*"
	@${INQ} -d 5 -collection test.$* -cc $< > $@ 2>&1 \
		&& echo " : pass" || echo " : fail"

test:	${OUTS}
clean:
	rm -f ${OUTS}

.PHONY:	clean

.include <bsd.obj.mk>
