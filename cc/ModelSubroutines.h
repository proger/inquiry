/*	$SYSREVERSE: ModelSubroutines.h,v 1.7 2011/05/05 19:48:18 proger Exp $	*/

#ifndef _SYSREV_INQUIRY_AST_MODELSUBR_H_
#define _SYSREV_INQUIRY_AST_MODELSUBR_H_

#include <clang/AST/Decl.h>
#include <clang/AST/DeclGroup.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Expr.h>
#include <clang/AST/Type.h>
#include <clang/Sema/DeclSpec.h>

namespace inquiry { namespace AST {

using namespace clang;
using std::string;

static inline Model::Location
Location(clang::PresumedLoc &ploc)
{
	Model::Location o(string(ploc.getFilename()), ploc.getLine(), ploc.getColumn());
	return o;
}

static inline Model::Location
Location(clang::SourceManager &sm, clang::SourceLocation loc)
{
	clang::PresumedLoc	ploc = sm.getPresumedLoc(loc);
	return Location(ploc);
}

static inline unsigned short
TagTypeKindToModelType(clang::TagTypeKind ttk)
{
	switch (ttk) {
	case clang::TTK_Struct:
		return (TYPE_STRUCT);
	case clang::TTK_Union:
		return (TYPE_UNION);
	case clang::TTK_Enum:
		return (TYPE_ENUM);
	default:
		/* class is not supported atm */
		return (0);
	};
}

#define TYPESPEC_UNCLOAK_STRUCTURE(type, vspec, vdecl)	do {		\
	if (const TagType *rt = (type)->getAs<TagType>()) {		\
		vspec = TagTypeKindToModelType(				\
		    rt->getDecl()->getTagKind());			\
		vdecl = rt->getDecl();					\
	} else if (const TypedefType *tt = (type)->getAs<TypedefType>()) {\
		vspec = TYPE_TYPEDEF;					\
		vdecl = tt->getDecl();					\
	} else {							\
		vdecl = NULL;						\
		break;							\
	}								\
} while (0);

} /* namespace AST */ } /* namespace inquiry */

#endif /* _SYSREV_INQUIRY_AST_MODELSUBR_H_ */
