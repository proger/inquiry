#	$SYSREVERSE: bsd.clang.mk,v 1.1 2011/05/04 08:32:25 proger Exp $

.include "${NETBSDSRCDIR}/external/bsd/llvm/Makefile.inc"

CPPFLAGS+=	-I${CLANG}/include 			\
		-I${LLVM}/include

#CLANG_RES=	${LLVMPREFIX}/lib/clang/2.8
#CXXFLAGS+=	-DCLANG_RESOURCE_PATH=\"${CLANG_RES}\"

CLANG_LIBS+= \
	clangFrontendTool \
	clangFrontend \
	clangStaticAnalyzerFrontend \
	clangStaticAnalyzerCheckers \
	clangStaticAnalyzerCore \
	clangDriver \
	clangSerialization \
	clangCodeGen \
	clangParse \
	clangSema \
	clangAnalysis \
	clangIndex \
	clangRewrite \
	clangAST \
	clangLex \
	clangBasic

LLVM_LIBS+= \
	AsmParser \
	BitReader \
	BitWriter \
	X86CodeGen \
	X86TargetInfo \
	X86Utils \
	X86AsmParser \
	X86Disassembler \
	X86AsmPrinter \
	SelectionDAG \
	AsmPrinter \
	CodeGen \
	Target \
	InstCombine \
	ScalarOpts \
	Analysis \
	MCDisassembler \
	MCParser \
	MC \
	ipo \
	TransformsUtils \
	ipa \
	Core \
	Support

.include "${NETBSDSRCDIR}/external/bsd/llvm/link.mk"
