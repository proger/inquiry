/*	$SYSREVERSE: inquiry.cc,v 1.27 2011/05/14 19:47:04 proger Exp $	*/
  
#include <iostream>
#include <sstream>
#include <bits/stl_pair.h>
#include <signal.h>

#include <llvm/LLVMContext.h>
#include <llvm/ADT/OwningPtr.h>
#include <llvm/ADT/Statistic.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/Timer.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetSelect.h>

#include <clang/Basic/Diagnostic.h>
#include <clang/Driver/Arg.h>
#include <clang/Driver/ArgList.h>
#include <clang/Driver/CC1Options.h>
#include <clang/Driver/DriverDiagnostic.h>
#include <clang/Driver/OptTable.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/FrontendDiagnostic.h>
#include <clang/Frontend/FrontendPluginRegistry.h>
#include <clang/Frontend/TextDiagnosticBuffer.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Rewrite/FrontendActions.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Parse/ParseAST.h>

#include <types.h>

#include <model/Artifacts.h>
#include <model/Component.h>
#include <model/TranslationUnit.h>
#include <model/Document.h>
#include <cc/MacroCallbacks.h>
#include <cc/ReferenceBuilder.h>

int		inqdebug = 0;

static void	LLVMErrorHandler(void *UserData, const std::string &Message);
static void	sighandler(int sig);

namespace inquiry {

using namespace clang;

class ASTReferenceAnalysisAction : public ASTFrontendAction {
public:
	void ExecuteAction() {
		CompilerInstance &CI = getCompilerInstance();

		ParseAST(CI.getPreprocessor(), &CI.getASTConsumer(), CI.getASTContext(),
		    CI.getFrontendOpts().ShowStats,
		    usesCompleteTranslationUnit(), 0);
	}

	void EndSourceFile() {
		Model::TU->leaveContext();
		delete takeCurrentASTUnit();
	}

protected:
	ASTConsumer *CreateASTConsumer(CompilerInstance &CI, llvm::StringRef inFile) {
		Preprocessor		&PP = CI.getPreprocessor();
		PP::MacroCallbacks	*MC;

		Model::TU = Model::TranslationUnit::create(inFile.str());
		Model::TU->enterContext();

		MC = new PP::MacroCallbacks();
		MC->Initialize(CI.getSourceManager());
		MC->PP = &PP;

		PP.addPPCallbacks(MC);

		AST::ReferenceASTConsumer *RAC = new AST::ReferenceASTConsumer();
		RAC->PP = &PP;
		RAC->MC = MC;

		std::cout << "processing: " << inFile.str() << std::endl;
		return (RAC);
	}
};

class Invocation : public CompilerInvocation {
public:
	static void Create(CompilerInvocation &Res, int argc, char **argv, Diagnostic &Diags) {
		/*
		 * All arguments past -cc are clang driver arguments.
		 * The others before are for inquiry flags.
		 */
		int	i, j;

		for (i = 0; i < argc; i++) {
			if (strcmp(argv[i], "-cc") == 0)
				break;
		}
		if (i >= (argc - 1)) {
			std::cerr << "no -cc arguments found\n";
			exit(1);
		}
		for (j = 0; j < i; j++) {
			if (strcmp(argv[j], "-collection") == 0 &&
			    (j + 1) < i)
				Model::mongo_collection = string(argv[j + 1]);
			else if (strcmp(argv[j], "-d") == 0 &&
			    (j + 1) < i)
				inqdebug = atoi(argv[j + 1]);
		}

		Invocation::CreateFromArgs(Res,
		    const_cast<const char **>(&argv[i + 1]),
		    const_cast<const char **>(&argv[argc]),
		    Diags);
	}
};

} /* namespace inquiry */

using namespace clang;
using std::cerr;

int
main(int argc, char **argv)
{
	llvm::OwningPtr<CompilerInstance>	 Inq(new CompilerInstance());
	llvm::IntrusiveRefCntPtr<DiagnosticIDs>	 DiagID(new DiagnosticIDs());
	TextDiagnosticBuffer			*DiagsBuffer = new TextDiagnosticBuffer();
	Diagnostic				 Diags(DiagID, DiagsBuffer);

	signal(SIGUSR1, sighandler);
	signal(SIGUSR2, sighandler);

	inquiry::Invocation::Create(Inq->getInvocation(), argc - 1, argv + 1, Diags);

	/*
	 * Infer the builtin include path if unspecified.
	 */
	if (Inq->getHeaderSearchOpts().UseBuiltinIncludes &&
			Inq->getHeaderSearchOpts().ResourceDir.empty())
#ifdef CLANG_RESOURCE_PATH
		/*
		 * Workaround for builds with misspecified resource paths.
		 * 	(which contain compiler configuration includes)
		 */
		Inq->getHeaderSearchOpts().ResourceDir = CLANG_RESOURCE_PATH;
#else
		Inq->getHeaderSearchOpts().ResourceDir =
			CompilerInvocation::GetResourcesPath(argv[0], reinterpret_cast<void *>(&main));
#endif

	Inq->createDiagnostics(argc, argv);

	llvm::install_fatal_error_handler(
	    LLVMErrorHandler,
	    static_cast<void *>(&Inq->getDiagnostics())
	);

	DiagsBuffer->FlushDiagnostics(Inq->getDiagnostics());

	bool				 rc = false;
	llvm::OwningPtr<FrontendAction>	 Act(new inquiry::ASTReferenceAnalysisAction());
	rc = Inq->ExecuteAction(*Act);

	llvm::llvm_shutdown();

	inquiry::PP::PostProcessMacros();

	inquiry::Model::DumpAll();

	inquiry::Model::MapComponents();
	inquiry::Model::DumpDocuments();

	cerr << "\n";
	if (!rc) {
		cerr << "reference analysis action had difficulties\n";
		cerr << "frontend diagnostic errors: "
		     << Inq->getDiagnosticClient().getNumErrors() << "\n";
	}
	cerr << "processed:   " << inquiry::Model::TranslationUnits.size()
				<< " translation units\n";
	cerr << "             " << inquiry::Model::AllFiles.size()
				<< " files total\n";
	cerr << "exported:    " << inquiry::Model::DMap.size() << " documents\n";

	return 0;
}

static void
LLVMErrorHandler(void *UserData, const std::string &Message)
{
	Diagnostic	&Diags = *static_cast<Diagnostic *>(UserData);

	Diags.Report(diag::err_fe_error_backend) << Message;
	cerr << "llvm error\n";
	exit(3);
}

static void
sighandler(int sig)
{
	switch (sig) {
	case SIGUSR1:
		inqdebug++;
		break;
	case SIGUSR2:
		inqdebug--;
		break;
	default:
		break;
	}
}
