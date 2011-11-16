/*	$SYSREVERSE: ReferenceBuilder.h,v 1.20 2011/04/19 15:47:33 proger Exp $	*/

#ifndef _SYSREV_INQUIRY_AST_REFBUILDER_H_
#define _SYSREV_INQUIRY_AST_REFBUILDER_H_

#include <clang/AST/Expr.h>

using std::string;
using std::list;
using std::pair;
using std::set;
using std::map;
using namespace clang;

namespace inquiry { namespace AST {

using Model::Component;
using Model::Structure;
using Model::Data;
using Model::Function;
using Model::ReferenceDesc;
using PP::MacroRecord;
using PP::MacroQueue;
using PP::MacroCallbacks;

class ReferenceASTConsumer : public ASTConsumer {
public:
	virtual void	 Initialize(ASTContext &context);
	virtual void	 HandleTranslationUnit(ASTContext &context);

	Preprocessor	*PP;
	SourceManager	*SM;
	MacroCallbacks	*MC;
};

const Type	*PurifyType(const Type *t);

} /* namespace AST */ } /* namespace inquiry */

#endif /* _SYSREV_INQUIRY_AST_REFBUILDER_H_ */
