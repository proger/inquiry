#	$SYSREVERSE: Makefile,v 1.22 2011/05/04 10:43:25 proger Exp $

.include <bsd.own.mk>

PROG_CXX=	inq
PROG=		inq

SRCS=		inquiry.cc
.PATH: ${.CURDIR}/cc
SRCS+=		ReferenceBuilder.cc MacroCallbacks.cc
.PATH: ${.CURDIR}/model
SRCS+=		Component.cc Artifacts.cc Document.cc

COPTS=
CFLAGS=

CXX?=		g++	
CXXFLAGS+=	-g -pipe
CXXFLAGS+=	-Wall -Werror -Woverloaded-virtual -Wcast-qual
CXXFLAGS+=	-Wall -Woverloaded-virtual -Wcast-qual
CXXFLAGS+=	-fno-exceptions -fno-rtti

CPPFLAGS+=	-I${.CURDIR}	\
		-D_DEBUG

MONGODRIVER?=	/home/proger/dev/mongo-c-driver
MONGOCPREFIX?=	/home/proger/dev/mongo-c-driver/src

CPPFLAGS+=	-I${MONGOCPREFIX}
LDFLAGS+=	-L${MONGODRIVER}
LDADD+=		-lstdc++ 			\
		${MONGODRIVER}/libbson.a	\
		${MONGODRIVER}/libmongoc.a

.include "bsd.localclang.mk"
#.include "bsd.clang.mk"
.include <bsd.prog.mk>
