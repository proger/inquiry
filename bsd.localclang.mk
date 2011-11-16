#	$SYSREVERSE: bsd.localclang.mk,v 1.1 2011/05/04 08:32:25 proger Exp $

LLVM=		${HOME}/dev/llvm
LLVMPREFIX=	${LLVM}/Debug+Asserts
CLANG=		${LLVM}/tools/clang

CPPFLAGS+=	-I${CLANG}/include 			\
		-I${LLVM}/include

CLANG_RES=	${LLVMPREFIX}/lib/clang/3.0
CPPFLAGS+=	-DCLANG_RESOURCE_PATH=\"${CLANG_RES}\"
CPPFLAGS+=	-D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS

LDADD+=		-lLLVM-3.0svn -lclang
LDFLAGS+=	-L${LLVMPREFIX}/lib -Wl,-rpath,${LLVMPREFIX}/lib
