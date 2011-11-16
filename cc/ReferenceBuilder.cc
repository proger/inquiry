/*	$SYSREVERSE: ReferenceBuilder.cc,v 1.63 2011/05/26 17:42:48 proger Exp $	*/

#include <iostream>
#include <sstream>
#include <string>
#include <bits/stl_pair.h>
#include <list>
#include <map>
#include <set>

#include <llvm/ADT/SmallString.h>
#include <clang/Basic/SourceManager.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclGroup.h>
#include <clang/AST/DeclVisitor.h>
#include <clang/AST/Stmt.h>
#include <clang/AST/Expr.h>
#include <clang/AST/TypeLoc.h>
#include <clang/AST/NestedNameSpecifier.h>
#include <clang/Sema/DeclSpec.h>

#include <types.h>

#include <model/Artifacts.h>
#include <model/Component.h>
#include <model/TranslationUnit.h>

#include "ModelSubroutines.h"
#include "MacroCallbacks.h"
#include "ReferenceBuilder.h"

#define RFBUILDER_DEBUG

#ifdef RFBUILDER_DEBUG
extern int inqdebug;

#define logdecl(D)	do {							\
		const char *kind = static_cast<Decl *>(D)->getDeclKindName();	\
		(D)->getLocation().dump(*SM);					\
		cerr << " " << __FUNCTION__ << " " << kind << "\n";\
	} while (0)
#define DEBUG(expr)	do {							\
		if (inqdebug > 2) { cerr << __FUNCTION__ << ": " << expr; }	\
	} while (0)
#define DEBUGE(expr)	do {							\
		if (inqdebug > 2) { expr; }					\
	} while (0)
#else
#define logdecl(D)
#define DEBUG(expr)
#define DEBUGE(expr)
#endif

namespace inquiry { namespace AST {

using std::cerr;
using std::ostringstream;
using std::pair;
using std::make_pair;
using namespace clang;

using Model::Component;
using Model::Structure;
using Model::Data;
using Model::Function;
using Model::ReferenceDesc;
using Model::Ref;
using PP::Macro;
using PP::MacroRefSet;
using PP::MacroRecord;
using PP::MacroQueue;

struct RelatedMacroHandler {
	virtual void	Found(MacroRecord *mr, bool idmatch = false) {
	}

	virtual ~RelatedMacroHandler() {}
};

class ReferenceBuilder : public DeclVisitor<ReferenceBuilder> {
public:
	ReferenceBuilder(ASTContext *_context, Preprocessor *_pp, MacroCallbacks *_mc, MacroQueue *_queue)
	    : context(_context), PP(_pp), MC(_mc), MRQ(_queue), _currentStructure(NULL) {
		    SM = &context->getSourceManager();
	}

	void	 VisitDecl(Decl *D) {}
	void	 VisitDeclContext(DeclContext *D);

	void	 VisitFunctionDecl(FunctionDecl *D);

	void	 VisitVarDecl(VarDecl *D);

	void	 VisitTypedefDecl(TypedefDecl *D);

	void	 VisitRecordDecl(RecordDecl *D, MacroQueue *mq = NULL);
	void	 VisitEnumDecl(EnumDecl *D);

	void	 RenameType(TypeDecl *tdv, string n);

	bool	 DiscoverTypeReferences(ReferenceDesc ref, Component *cm,
					const Type *t, FunctionDecl *fd = NULL);

	SourceRange GetDeclRange(const Decl *d);

	bool	 LookupMacroExpansions(MacroQueue *mq, Component *target,
					SourceRange range, int refarg = -1);
	bool	 LookupMacroExpansions(MacroQueue *mq, Component *target,
					SourceRange range, ReferenceDesc ref);
	int	 TestMacroRecord(MacroRecord *MR, Component *target, SourceRange testRange);

	void	 TraverseStmtChildren(Stmt *stmt, Component *cm, MacroQueue *mq,
				ReferenceDesc rd, int rlevel = 0);
	void	 TraverseFields(const RecordDecl *rd, Structure *sr, MacroQueue *mq);
	void	 TraverseEnumeration(const EnumDecl *ed, Structure *sr);
	void	 TraverseInitializer(const InitListExpr *ile, Data *global, MacroQueue *mq);

	void	 HandleCastExpr(ExplicitCastExpr *ece);

	bool	 FindRelatedMacros(Decl *D, Component *C, RelatedMacroHandler *RMH);

protected:
	ASTContext	*context;
	SourceManager	*SM;
	Preprocessor	*PP;
	MacroCallbacks	*MC;
	MacroQueue	*MRQ;

	Structure	*_currentStructure;
};

void
ReferenceBuilder::VisitDeclContext(DeclContext *D)
{
	DeclContext::decl_iterator	DI, DE;

	for (DI = D->decls_begin(), DE = D->decls_end(); DI != DE; DI++) {
		if ((*DI)->isImplicit())
			continue;

		if (NamedDecl *ND = dyn_cast<NamedDecl>(*DI)) {
			SourceLocation	loc = ND->getLocation();

			if (!loc.isValid() || (loc.isFileID() &&
			    strcmp(SM->getPresumedLoc(loc).getFilename(), "<built-in>") == 0))
				continue;
		}

		Visit(*DI);
	}
}

void
ReferenceBuilder::VisitFunctionDecl(FunctionDecl *D)
{
	Function	*function;
	string		 id = D->getNameAsString();
	Model::Location	 location = Location(*SM, D->getLocation());
	unsigned char	 flags = 0;
	CompoundStmt	*body = NULL;

	DEBUG(id << "\n");
	    
	if (D->getStorageClass() == SC_Static) 
		flags |= Component::TULocal;

	function = Model::lookup<Function>(FUNCTION_REG, id, flags);
	if (function == NULL) {
		function = new Function(FUNCTION_REG, id, flags);
		Model::bind(function, location.filename());

		function->returntype = D->getResultType().getCanonicalType().getAsString();
		DiscoverTypeReferences(Ref(REF_RETURN), function, &*D->getResultType());
	}

	if (D->isThisDeclarationADefinition()) {
		function->definition = location;

		body = dyn_cast<CompoundStmt>(D->getBody());
		assert(body != NULL);
		TraverseStmtChildren(body, function, NULL, Ref(REF_BODYENC));
	} else
		function->declarations.insert(location);

	if (function->arguments.size() == 0 || (!function->argscomplete && body)) {
		unsigned				i = 0;
		FunctionDecl::param_const_iterator	P, PEND;

		function->arguments.clear();
		for (P = D->param_begin(), PEND = D->param_end(); P != PEND; P++, i++) {
			ParmVarDecl	*vd = *P;
			QualType	 qt = vd->getType().getCanonicalType();

			if (DiscoverTypeReferences(Ref(REF_ARGUMENT, i), function, &*qt) == false)
				DEBUG("******* arg " << i << " "
					<< qt.getAsString() << " NOREF\n");

			function->arguments.push_back(make_pair(
			    qt.getAsString(),
			    vd->getQualifiedNameAsString()
			));
		}
		if (body)
			function->argscomplete = true;
	}

	struct FunctionMacroHandler : public RelatedMacroHandler {
		Function	*function;
		bool		 isdef;

		FunctionMacroHandler(Function *_f, bool _isdef)
		    : function(_f), isdef(_isdef) {
		}

		void Found(MacroRecord *mr, bool idmatch = false) {
			if (idmatch && !isdef) {
				DEBUG("   idmatch && !isdef: " << function->id
				      << " " << mr->macro->id << "\n");
				mr->macro->component->reference(function, Ref(REF_BODYENC));
			} else if (!idmatch && isdef) {
				DEBUG("   !idmatch && isdef: " << function->id
				      << " " << mr->macro->id << "\n");
				function->reference(mr->macro->component, Ref(REF_BODYENC));
			} else
				DEBUG("    MATCH MISSING " << function->id
				      << " " << mr->macro->id << "\n");
		}
	} handler(function, D->isThisDeclarationADefinition());

	FindRelatedMacros(D, function, &handler);
}

void
ReferenceBuilder::VisitVarDecl(VarDecl *D)
{
	if (D->hasGlobalStorage() == false)
		return;

	string			 id = D->getNameAsString();
	Data			*global;
	Model::Location		 location = Location(*SM, D->getLocation());
	unsigned char		 flags = 0;
	bool			 found = false;

	DEBUG(id << "\n");

	if (D->getStorageClass() == SC_Static) 
		flags |= Component::TULocal;

	global = Model::lookup<Data>(DATA_GLOBAL, id, flags);
	if (global) {
		found = true;
	} else {
		global = new Data(DATA_GLOBAL, id, flags);
		Model::bind(global, location.filename());
	}

	VarDecl::redecl_iterator	rdi, rde;
	for (rdi = D->redecls_begin(), rde = D->redecls_end(); rdi != rde; rdi++) {
		VarDecl		*RD = *rdi;
		Model::Location	 rlocation = Location(*SM, RD->getLocation());
		if (RD->getStorageClass() == SC_Extern || !RD->isThisDeclarationADefinition())
			global->declarations.insert(rlocation);
		else
			global->definition = rlocation;
	}
	if (found)
		return;

	global->datatype = D->getType().getCanonicalType().getAsString();

	const Type	*type = PurifyType(&*D->getType());
	int		 componentType;
	TypeDecl	*typeDecl;

	TYPESPEC_UNCLOAK_STRUCTURE(type, componentType, typeDecl);
	if (typeDecl && typeDecl->getName().empty()) {
		Structure	*anon;
		ostringstream	 tid; tid << global->id << "::anon";

		RenameType(typeDecl, tid.str());

		if (isa<RecordDecl>(typeDecl))
			VisitRecordDecl(dyn_cast<RecordDecl>(typeDecl));
		else
			VisitEnumDecl(dyn_cast<EnumDecl>(typeDecl));

		anon = _currentStructure;
		anon->parent = global;
		global->reference(anon, Ref(REF_TYPE));
	} else
		DiscoverTypeReferences(Ref(REF_TYPE), global, type);

	struct InitializerMacroHandler : public RelatedMacroHandler {
		MacroQueue	 related;
		Data		*global;

		InitializerMacroHandler(Data *_d)
		    : global(_d) {}

		void Found(MacroRecord *mr, bool idmatch = false) {
			if (idmatch)
				mr->macro->component->reference(global, Ref(REF_MACRODECL));
			else
				related.push_back(*mr);
		}
	} handler(global);
	
	FindRelatedMacros(D, global, &handler);

	Expr *e = const_cast<Expr *>(D->getAnyInitializer());

	if (D->isThisDeclarationADefinition() && e != NULL) {
		InitListExpr *ile = dyn_cast<InitListExpr>(e);
		if (ile)
			TraverseInitializer(ile, global, &handler.related);
	}

	return;
}

void
ReferenceBuilder::VisitTypedefDecl(TypedefDecl *D)
{
	string		 id = D->getNameAsString();
	Model::Location	 location = Location(*SM, D->getLocation());
	Structure	*type;
	Structure	*underlying = NULL;

	DEBUG(id << "\n");

	const Type *ty = &*D->getUnderlyingType();
	if (const TagType *tt = ty->getAs<TagType>()) {
		string		 name = tt->getDecl()->getNameAsString();
		int		 mtype = TagTypeKindToModelType(tt->getDecl()->getTagKind());

		if (name.size()) {
			underlying = Model::lookup<Structure>(mtype, name);
			if (underlying == NULL) {
				if (isa<RecordDecl>(tt->getDecl()))
					VisitRecordDecl(dyn_cast<RecordDecl>(tt->getDecl()));
				else
					VisitEnumDecl(dyn_cast<EnumDecl>(tt->getDecl()));

				underlying = Model::lookup<Structure>(mtype, name);
			}
			assert(underlying != NULL);
		} else {
			ostringstream	 uid;
			uid << id << "::typedef";
			RenameType(tt->getDecl(), uid.str());

			if (isa<RecordDecl>(tt->getDecl()))
				VisitRecordDecl(dyn_cast<RecordDecl>(tt->getDecl()));
			else
				VisitEnumDecl(dyn_cast<EnumDecl>(tt->getDecl()));

			underlying = Model::lookup<Structure>(mtype, uid.str());
		}
	} else if (const TypedefType *tt = ty->getAs<TypedefType>()) {
		underlying = Model::lookup<Structure>(TYPE_TYPEDEF, tt->getDecl()->getNameAsString());
	}
       
	type = Model::lookup<Structure>(TYPE_TYPEDEF, id);
	if (type != NULL)
		return;

	type = new Structure(TYPE_TYPEDEF, id);
	Model::bind(type, location.filename());

	type->definition = location;

	if (underlying != NULL) {
		type->reference(underlying, Ref(REF_UNDERLAYER));
		underlying->parent = type;
	}
}

void
ReferenceBuilder::VisitRecordDecl(RecordDecl *D, MacroQueue *mq)
{
	Structure	*struc = NULL;
	int		 type = TagTypeKindToModelType(D->getTagKind());
	string		 id = D->getNameAsString();
	MacroQueue	*queuep = mq;

	DEBUG(id << "\n");
	if (id.length() == 0)
		return;

	Model::Location	 loc = Location(*SM, D->getLocation());

	_currentStructure = struc = Model::lookup<Structure>(type, id);
	if (struc == NULL) {
		_currentStructure = struc = new Structure(type, id);
		Model::bind(struc, loc.filename());
	}

	if (D->isDefinition()) {
		struc->definition = loc;

		if (_currentStructure->fields.size())
			return;
	} else {
		struc->declarations.insert(loc);
		return;
	}

	struct RecordMacroHandler : public RelatedMacroHandler {
		MacroQueue	related;
		void Found(MacroRecord *mr, bool idmatch = false) { related.push_back(*mr); }
	} handler;

	if (mq == NULL) {	/* top level */
		/* called during traversal */
		FindRelatedMacros(D, _currentStructure, &handler);
		queuep = &handler.related;
	} else {
		/* called during anonymous type initialization */
	}

	TraverseFields(D, _currentStructure, queuep);

	if (mq == NULL) {	/* top level */
		/* walk any left macros and make sr reference them. */
		MacroQueue::iterator		 MRI, MRE;
		for (MRI = queuep->begin(), MRE = queuep->end(); MRI != MRE; MRI++) {
			MacroRecord	*mr = &*MRI;

			_currentStructure->reference(
			    mr->macro->component, Ref(REF_BODYENC));
		}
	}
}

void
ReferenceBuilder::VisitEnumDecl(EnumDecl *D)
{
	Structure	*struc = NULL;
	int		 type = TagTypeKindToModelType(D->getTagKind());
	string		 id = D->getNameAsString();

	Model::Location	 loc = Location(*SM, D->getLocation());

	DEBUG(type << " " << id << "\n");
	if (id.size() != 0) {
		_currentStructure = struc = Model::lookup<Structure>(type, id);
		if (struc == NULL) {
			_currentStructure = struc = new Structure(type, id);
			Model::bind(struc, loc.filename());
		}

		if (D->isDefinition())
			struc->definition = loc;
		else
			struc->declarations.insert(loc);

		if (_currentStructure->bodycomplete)
			return;
	} else
		_currentStructure = NULL;

	EnumDecl::enumerator_iterator	IT, END;

	for (IT = D->enumerator_begin(), END = D->enumerator_end(); IT != END; IT++) {
		EnumConstantDecl	*ecd = *IT;
		llvm::SmallString<10>	 val;
		Model::Location		 location = Location(*SM, ecd->getLocation());

		Data *d = Model::lookup<Data>(DATA_ENUMVAL, ecd->getNameAsString());
		if (d == NULL) {
			d = new Data(DATA_ENUMVAL, ecd->getNameAsString());
			Model::bind(d, location.filename());
		}

		ecd->getInitVal().toString(val);
		d->value = val.str().str();
		d->definition = location;

		if (_currentStructure)
			_currentStructure->reference(d, Ref(REF_ENUM));
	}
	if (_currentStructure)
		_currentStructure->bodycomplete = true;
}

void
ReferenceBuilder::TraverseFields(const RecordDecl *rd, Structure *sr, MacroQueue *mq)
{
	RecordDecl::field_iterator	IT, END;
	int				i = 0, anons = 0;
	Model::DeclPairList		fields;

	for (IT = rd->field_begin(), END = rd->field_end(); IT != END; IT++, i++) {
		FieldDecl	*D = *IT;
		QualType	 fieldType = D->getType();
		const Type	*pureFieldType = PurifyType(&*fieldType.getCanonicalType());

		/*
		 * Handle anonymous types
		 */
		const TagType	*childType = dyn_cast<TagType>(pureFieldType);
		TagDecl		*childTypeDecl = childType ? childType->getDecl() : NULL;

		if (childTypeDecl && childTypeDecl->getName().empty()) {
			ostringstream	 id;
			id << sr->id << "::anon";
			if (anons != 0)
				id << anons;
			string name = id.str();

			RenameType(childTypeDecl, name);

			if (isa<RecordDecl>(childTypeDecl))
				VisitRecordDecl(dyn_cast<RecordDecl>(childTypeDecl), mq);
			else
				VisitEnumDecl(dyn_cast<EnumDecl>(childTypeDecl));

			Structure *anon = Model::lookup<Structure>(
				TagTypeKindToModelType(childTypeDecl->getTagKind()),
				name);
			assert(anon->id == name);
			anon->parent = sr;

			anons++;
		}

		/*
		 * Explicitly tell the TypePrinter to suppress type scope
		 * 	(which is always empty in C except for anon types),
		 * because our anonymous typenames already contain the parent scope.
		 */
		LangOptions	 LO;
		PrintingPolicy	 PP(LO);
		PP.SuppressScope = 1;

		sr->fields.push_back(make_pair(fieldType.getAsString(PP), D->getNameAsString()));

		DEBUG("  field " << D->getNameAsString() << "\n");

		LookupMacroExpansions(mq, sr, GetDeclRange(D), i);

		assert(DiscoverTypeReferences(Ref(REF_FIELD, i), sr, pureFieldType));
	}
}

bool
ReferenceBuilder::DiscoverTypeReferences(ReferenceDesc ref, Component *cm, 
					 const Type *t, FunctionDecl *fd)
{
	const Type	*ref_type = PurifyType(t);
	bool		 ret = true;

	if (const FunctionProtoType *ft = dyn_cast<FunctionProtoType>(ref_type)) {
		if (fd == NULL)
			return true;
		/*
		 * If discovering for a function:
		 * extract all argument types for references.
		 */
		for (unsigned i = 0; i < ft->getNumArgs(); i++) {
			QualType arg = ft->getArgType(i).getCanonicalType();
			ret = DiscoverTypeReferences(ref, cm, &*arg, fd);
		}

		Function *f = Model::lookup<Function>(FUNCTION_REG, fd->getNameAsString());
		if (!f)
			return false;
		cm->reference(f, ref);
	} else {
		int		 mtype;
		NamedDecl	*ref_decl = NULL;

		TYPESPEC_UNCLOAK_STRUCTURE(ref_type, mtype, ref_decl);
		if (ref_decl != NULL) {
			string name = ref_decl->getNameAsString();
			Structure *refobj = Model::lookup<Structure>(mtype, name);
			if (!refobj) {
				Visit(ref_decl);
				refobj = Model::lookup<Structure>(mtype, name);
			}
			if (!refobj) {
#ifdef RFBUILDER_DEBUG
				cerr << "**** bad refobj: <" << name << ">";
				cerr << " decl: "; ref_decl->getLocation().dump(*SM);
				cerr << " from component: " << cm->id << "\n";
#endif
				return false;
			}
			cm->reference(refobj, ref);
		}
	}

	return ret;
}

/*
 * called when processing declarations
 *
 * processing declarations is linear, so it's safe to remove
 * early macros
 */
bool
ReferenceBuilder::LookupMacroExpansions(MacroQueue *mq, Component *target, SourceRange range, int refarg)
{
	int			 test;
	MacroQueue::iterator	 MRI, MRE;

	for (MRI = mq->begin(), MRE = mq->end(); MRI != MRE; MRI++) {
		MacroRecord	*mr = &*MRI;

		test = TestMacroRecord(mr, target, range);
		if (test == 0) {
			ReferenceDesc ref = Ref(
			    mr->macro->function ? REF_MACRODECL : REF_MACROHINT,
			    refarg);

			target->reference(mr->macro->component, ref);
			mq->erase(MRI);
		} else if (test == -1) {
			/* TODO: test */
			mq->erase(MRI);
		} else if (test == 1) {
			return true;
		}
	}

	return true;
}

int
ReferenceBuilder::TestMacroRecord(MacroRecord *MR, Component *target, SourceRange testRange)
{
	bool		beforeend, afterstart;

	if (MR->location == testRange.getBegin())
		return 0;
	if (MR->location == testRange.getEnd())
		return 0;

	beforeend = SM->isBeforeInTranslationUnit(MR->location, testRange.getEnd());
	afterstart = SM->isBeforeInTranslationUnit(testRange.getBegin(), MR->location);
	if (beforeend && afterstart)
		return 0;

	if (afterstart)
		return 1;
	if (beforeend)
		return -1;

	return 2;
}

void
ReferenceBuilder::TraverseStmtChildren(Stmt *stmt, Component *cm, MacroQueue *mq,
					ReferenceDesc rd, int rlevel)
{
	Stmt::child_iterator	IT, END;

	for (IT = stmt->child_begin(), END = stmt->child_end(); IT != END; IT++) {
		if (*IT == NULL)
			continue;
#ifdef RFBUILDER_DEBUG
		if (inqdebug > 3) {
			for (int i = 0; i < rlevel; i++)
				cerr << " ";
			cerr << "stmt: " << IT->getStmtClassName() << "\n";
		}
#endif
		TraverseStmtChildren(*IT, cm, mq, rd, rlevel + 1);

		if (DeclRefExpr *de = dyn_cast<DeclRefExpr>(*IT)) {
			ValueDecl	*decl = de->getDecl();
			QualType	 qt = decl->getType().getCanonicalType();

			if (VarDecl *vd = dyn_cast<VarDecl>(decl)) {
				if (vd->hasGlobalStorage() ||
				    vd->getStorageClass() == SC_Static) {
					Data *global = Model::lookup<Data>(DATA_GLOBAL,
					    vd->getNameAsString());

					if (global != NULL)
						cm->reference(global, rd);
				}
			} else if (FunctionDecl *fd = dyn_cast<FunctionDecl>(decl)) {
				Function *fn = Model::lookup<Function>(FUNCTION_REG,
				    fd->getNameAsString());

				if (fn != NULL)
					cm->reference(fn, rd);
			} else if (ValueDecl *vd = dyn_cast<ValueDecl>(decl)) {
				Data *enumval = Model::lookup<Data>(DATA_ENUMVAL,
				    vd->getNameAsString());

				if (enumval)
					cm->reference(enumval, rd);
			}
		} else if (MemberExpr *me = dyn_cast<MemberExpr>(*IT)) {
			FieldDecl *fd = dyn_cast<FieldDecl>(me->getMemberDecl());
			if (fd == NULL)
				continue;

			RecordDecl *rd = fd->getParent()->getDefinition();
			int type = TagTypeKindToModelType(rd->getTagKind());

			Structure *s = Model::lookup<Structure>(type, rd->getNameAsString());
			if (s == NULL)
				continue;

			Model::DeclPairList::iterator IT, END;
			int i = 0;	/* TODO: eliminate linear search */
			for (IT = s->fields.begin(), END = s->fields.end();
			    IT != END;
			    IT++, i++)
				if (IT->second == fd->getNameAsString()) {
					cm->reference(s, Ref(REF_FIELDACCESS, i));
					break;
				}

		} else if (ExplicitCastExpr *ece = dyn_cast<ExplicitCastExpr>(*IT)) {
			QualType	 qt = ece->getTypeAsWritten().getCanonicalType();

			DiscoverTypeReferences(rd, cm, &*qt);
			HandleCastExpr(ece);
		}
	}

	if (mq != NULL) {
		LookupMacroExpansions(mq, cm, stmt->getSourceRange(), rd);
	}
}

/*
 * called when traversing statements
 *
 * traversing statements is recursive and this function
 * is called in the tail, so removing early macros is bad here.
 */
bool
ReferenceBuilder::LookupMacroExpansions(MacroQueue *mq, Component *target,
					SourceRange range, ReferenceDesc ref)
{
	MacroQueue::iterator	 IT, END;

	range.setBegin(SM->getInstantiationLoc(range.getBegin()));
	range.setEnd(SM->getInstantiationLoc(range.getEnd()));

	for (IT = mq->begin(), END = mq->end(); IT != END; IT++) {
		MacroRecord	*mr = &*IT;
		int		 test;

		test = TestMacroRecord(mr, target, range);
		if (test == 0) {
			target->reference(mr->macro->component, ref);
			mq->erase(IT);
		} else if (test == 1) {
			return false;
		}
	}

	return true;
}

void
ReferenceBuilder::TraverseInitializer(const InitListExpr *ile, Data *global, MacroQueue *mq)
{
	unsigned	 i, k;
	InitListExpr	*syn = ile->getSyntacticForm();

	Structure	*lastcom = NULL;
	RecordDecl	*lastrecord = NULL;
	int		 lastfindex = -1;
	Model::DeclPairList::iterator lastdli;

	if (syn == NULL)
		return;

	for (i = 0; i < syn->getNumInits(); i++) {
		Expr			*init = syn->getInit(i);
		DesignatedInitExpr	*di = dyn_cast<DesignatedInitExpr>(init);
		if (di == NULL) {			/* regular style */
			TraverseStmtChildren(init, global, mq, Ref(REF_INITIALIZER, i));
			continue;
		}
		for (k = 0; k < di->getNumSubExprs(); k++) {	/* c99 style */
			DesignatedInitExpr::Designator	*d = di->getDesignator(k);

			if (d->isFieldDesignator() == false)
				continue;

			FieldDecl	*f = d->getField();
			Expr		*e = di->getSubExpr(k);
			RecordDecl	*r;
			Structure	*s;

			int		 findex = 0;
			string		 fname = f->getNameAsString();

			if ((r = f->getParent()) == lastrecord) {
				s = lastcom;	
			} else {
				int comtype = TagTypeKindToModelType(r->getTagKind());
				string name = r->getNameAsString();

				lastcom = s = Model::lookup<Structure>(comtype, name);
				lastdli = s->fields.begin();
			}
			lastrecord = r;

			/*
			 * Map the field name to the field index for referencing.
			 * Yields: findex.
			 */
			if (lastdli != s->fields.end() && lastdli->second == fname)
				findex = lastfindex += 1;
			else {	/* slow path */
				Model::DeclPairList::iterator dpi, dpe;
				for (dpi = s->fields.begin(), dpe = s->fields.end();
				    dpi != dpe; dpi++, findex++)
					if (dpi->second == fname) {
						lastfindex = findex;
						lastdli = ++dpi;
						break;
					}
			}

			TraverseStmtChildren(e, global, mq, Ref(REF_INITIALIZER, findex));
		}
	}
}

void
ReferenceBuilder::HandleCastExpr(ExplicitCastExpr *ece)
{
	QualType	 qt = ece->getTypeAsWritten().getCanonicalType();
	DeclRefExpr	*subjexpr = dyn_cast<DeclRefExpr>(ece->getSubExpr());
	if (!subjexpr)
		return;
	/* working only with values as subjects */
	ValueDecl	*vsubj = dyn_cast<ValueDecl>(subjexpr->getDecl());
	if (!vsubj)
		return;

	/*
	 * cast target references cast subject
	 * struct device *dev;		-> subject
	 * (struct em_softc *)dev;	-> target
	 */
	QualType	 st = vsubj->getType().getCanonicalType();
	int		 subjtype = -1, targtype = -1;
	NamedDecl	*subj = NULL, *targ = NULL;

	TYPESPEC_UNCLOAK_STRUCTURE(PurifyType(&*qt), targtype, targ);
	TYPESPEC_UNCLOAK_STRUCTURE(PurifyType(&*st), subjtype, subj);
	if (!targ || !subj)
		return;

	/*
	 * The reference is created only when the cast target
	 * is a structure which has subject as its beginning.
	 * (i.e. one structure encapsulates another)
	 */
	RecordDecl	*rd = dyn_cast<RecordDecl>(targ);
	if (rd == NULL)
		if (TypedefDecl *ttd = dyn_cast<TypedefDecl>(targ)) {
			const RecordType *rtarg = dyn_cast<RecordType>(&*ttd->getUnderlyingType());
			if (rtarg)
			    rd = rtarg->getDecl();
		}
	targ = NULL; /* we will allow it later if needed */
	if (rd != NULL) {
		RecordDecl::field_iterator f1 = rd->field_begin();
		if (f1 != rd->field_end()) {
			if (PurifyType(&*f1->getType()) == PurifyType(&*vsubj->getType()))
				targ = rd;
		}
	}

	if (targ) {
		Structure *tc = Model::lookup<Structure>(targtype, targ->getNameAsString());
		Structure *sc = Model::lookup<Structure>(subjtype, subj->getNameAsString());

		if (sc && tc)
			tc->reference(sc, Ref(REF_INHERITANCE));
	}
}

SourceRange
ReferenceBuilder::GetDeclRange(const Decl *D)
{
	SourceLocation		 begin = D->getLocStart();
	SourceLocation		 end = D->getLocEnd();

	if (begin.isMacroID() == false)
		end = PP->getLocForEndOfToken(end);

	if (const FieldDecl *FD = dyn_cast<FieldDecl>(D)) {
		if (const TypeSourceInfo *TSI = FD->getTypeSourceInfo())
			end = TSI->getTypeLoc().getEndLoc();

	} else if (const VarDecl *VD = dyn_cast<VarDecl>(D)) {
		const Expr *expr = VD->getAnyInitializer();

		if (VD->isThisDeclarationADefinition() && expr != NULL)
			end = expr->getSourceRange().getEnd();

	} else if (const FunctionDecl *FD = dyn_cast<FunctionDecl>(D)) {
		if (FD->isThisDeclarationADefinition() && FD->hasBody())
			end = FD->getBody()->getSourceRange().getEnd();
	}

	return SourceRange(SM->getInstantiationLoc(begin), SM->getInstantiationLoc(end));
}

void
ReferenceBuilder::RenameType(TypeDecl *tdv, string n)
{
	DEBUG("<" << tdv->getNameAsString() << "> into " << n << "\n");

	IdentifierInfo &II = context->Idents.get(n.c_str(), n.length());
	tdv->setDeclName(DeclarationName(&II));
}

bool
ReferenceBuilder::FindRelatedMacros(Decl *D, Component *C, RelatedMacroHandler *RMH)
{
	MacroQueue::iterator MRI, MRE;
	SourceRange      range = GetDeclRange(D);
#if 0
	cerr << "matching macros for component: " << C->id << ", " << "range: ";
	range.getBegin().dump(*SM); cerr << " -- "; range.getEnd().dump(*SM);
	cerr << "\n";
#endif
	if (!range.getBegin().isValid() || !range.getEnd().isValid())
		return false;

	for (MRI = MRQ->begin(), MRE = MRQ->end(); MRI != MRE; MRI++) {
		MacroRecord	*mr = &*MRI;
		SourceLocation	 sl = SM->getInstantiationLoc(mr->location);
#if 0
		cerr << "- macro: " << mr->macro->id << "\n";
#endif
		if (mr->macro->incomplete)
			MC->CompleteDefinition(mr->macro);

		/* Embedded use data macros no not need to be processed,
		 * MacroCallbacks has taken care of it. */
		if (!mr->macro->function && mr->location.isMacroID()) {
			MRQ->erase(MRI);
			continue;
		}

		if (sl != range.getBegin()) {
			if (SM->isBeforeInTranslationUnit(range.getEnd(), sl)) {
				// cerr << "  dr before sl; break\n";
				break;
			}

			if (SM->isBeforeInTranslationUnit(sl, range.getBegin())) {
				// cerr << "  dr after sl; erase\n";
				MRQ->erase(MRI);
				continue;
			}
		}

		set<string>::iterator   i;

		i = mr->macro->identifiers.find(C->id);
		if (i != mr->macro->identifiers.end()) {
			RMH->Found(mr, true);
			continue;
		}

		if (sl == range.getEnd() || sl == range.getBegin() ||
		    (SM->isBeforeInTranslationUnit(mr->location, range.getEnd()) &&
		     SM->isBeforeInTranslationUnit(range.getBegin(), mr->location))) {
			RMH->Found(mr);
		}
	}

	return true;
}

void
ReferenceASTConsumer::Initialize(ASTContext &context)
{
	SM = &context.getSourceManager();
}

void
ReferenceASTConsumer::HandleTranslationUnit(ASTContext &context)
{
	MacroQueue::iterator		 MRI, MRE;

	ReferenceBuilder RB(&context, PP, MC, &PP::MRQ);
	RB.VisitDeclContext(context.getTranslationUnitDecl());

	MRE = PP::MRQ.end();
	while ((MRI = PP::MRQ.begin()) != MRE) {
		MacroRecord	*mr = &*MRI;

		DEBUG("macro leftover: " << mr->macro->id << " ");
		DEBUGE(mr->location.dump(*SM); cerr << "\n");

		if (mr->macro->incomplete)
			MC->CompleteDefinition(mr->macro);
		PP::MRQ.pop_front();
	}
}

/*
 * Get a plain type reference: strip all pointers and arrays
 * e.g.: char **[3] -> char
 */
const Type *
PurifyType(const Type *t)
{
	do {
		if (const PointerType *p = dyn_cast<PointerType>(t))
			t = &*p->getPointeeType();
		else if (const ArrayType *a = dyn_cast<ArrayType>(t))
			t = &*a->getElementType();
		else
			break;
	} while (1);

	return (t);
}

} /* namespace AST */ } /* namespace inquiry */
