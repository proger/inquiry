/*	$SYSREVERSE: MacroCallbacks.cc,v 1.25 2011/05/14 19:47:04 proger Exp $	*/

#include <iostream>
#include <sstream>
#include <cstring>
#include <string>
#include <list>
#include <map>
#include <set>

#include <llvm/Support/raw_ostream.h>

#include <clang/Basic/SourceManager.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Basic/FileManager.h>

#include <clang/Lex/MacroInfo.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/PPCallbacks.h>

#include <types.h>

#include <model/Artifacts.h>
#include <model/Component.h>
#include <model/TranslationUnit.h>

#include "ModelSubroutines.h"
#include "MacroCallbacks.h"

#define PPDEBUG

#ifdef PPDEBUG
extern int inqdebug;
#define mout(mc) do { 					\
		if (inqdebug < 3) break;		\
		cerr << "macro: "; mc->dump(cerr);	\
		cerr << "definition: "; MI->getDefinitionLoc().dump(*SM); cerr << "\n"; \
	} while (0)
#else
#define mout(mc)
#endif

namespace inquiry { namespace PP {

using std::cerr;

using namespace clang;

using Model::File;
using Model::DeclPairList;
using Model::Ref;
using Model::ReferenceDesc;
using Model::Component;
using Model::Data;
using Model::Function;

MacroQueue	 MRQ;
MacroQueue	 MacroPostProcessQueue;

#define IS_BUILTIN_MACRO(sm, mi, loc)						\
			((mi)->isBuiltinMacro() || (				\
			    (loc).isFileID() &&					\
			    !strcmp((sm)->getPresumedLoc((loc)).getFilename(),	\
				"<built-in>")					\
			))

MacroObjMap	 MacroMap;

Macro *
mlookup(string _id, bool silent)
{
	Macro			*m = NULL;
	MacroObjMap::iterator	 IT;

	if ((IT = MacroMap.find(_id)) != MacroMap.end()) {
		m = IT->second;
	} else if (!silent) {
		m = new Macro(_id);
		MacroMap[_id] = m;
	}
	return (m);
}

void
Macro::reference(Macro *RHS, ReferenceDesc *rd)
{
	if (this->incomplete || RHS->incomplete) {
		pendingrefs.insert(RHS);
		return;
	}

	component->reference(RHS->component, rd ? *rd : Ref(REF_BODYENC));
}

void
MacroCallbacks::FileChanged(SourceLocation Loc,
			    FileChangeReason Reason,
			    SrcMgr::CharacteristicKind FileType)
{
	if (Reason == EnterFile && Loc.isValid() && Loc.isFileID()) {
		FileEntry const *ent = SM->getFileEntryForID(SM->getFileID(Loc));

		if (ent && inqdebug > 0)
			cerr << "preprocessing: " << ent->getName() << "\n";
		if (ent && !SM->isFromMainFile(Loc))
			Model::TU->include(ent->getName());
	}
}

void
MacroCallbacks::MacroDefined(const Token &Identifier, const MacroInfo *MI)
{
	if (IS_BUILTIN_MACRO(SM, MI, MI->getDefinitionLoc()))
		return;

	IdentifierInfo	*II = Identifier.getIdentifierInfo();
	string		 name(II->getNameStart(), II->getLength());
	Macro		*m = mlookup(name)->current();

	if (m->component)
		return;

	set<string>	 argset;
	ostringstream	 replacement;
	Macro		*repl = NULL;

	m->MI = const_cast<MacroInfo *>(MI);
	m->definition = MI->getDefinitionLoc();

	MacroInfo::arg_iterator	AIT, AEND;
	for (AIT = MI->arg_begin(), AEND = MI->arg_end(); AIT != AEND; AIT++) {
		string	argname((*AIT)->getNameStart(), (*AIT)->getLength());

		m->args.push_back(make_pair(string(), argname));
		argset.insert(argname);
	}

	/*
	 * Scan identifier tokens for references to other macros.
	 * Also assume that if the macro's first token is an identifier,
	 * it may substitute some other function implictly.
	 *
	 * Defer actual symbol resolving up to the first expansion since
	 * a substitutive macro can be defined way before.
	 */
	int 	i, lastconcat = -2;
	bool	l_paren_seen = false;
	for (i = 0; i < (int)MI->getNumTokens(); i++) {
		Token	 token = MI->getReplacementToken(i);
		string	 spell = PP->getSpelling(token);
		
		if (token.hasLeadingSpace())
			replacement << " ";
		replacement << spell;

		switch (token.getKind()) {
		case tok::identifier:
			if (argset.find(spell) == argset.end() &&
			    lastconcat != (i - 1)) {
				if ((repl = mlookup(spell, true)) != NULL)
					m->reference(repl);
				else if (i == 0) {
					/* macro to possible identifier */
					m->sub.valid = true;
					m->sub.identifier = spell;
				}
			}
			break;
		case tok::l_paren:	/* TODO: #define foo(a,b,c) (bar(a,b,c,0)) ? */
			if (!l_paren_seen && i != 1)
				m->sub.valid = false;
			l_paren_seen = true;
			break;
		case tok::semi:
			m->sub.valid = false;
			break;
		case tok::hashhash:
			lastconcat = i;
		default:
			break;
		}
	}
	m->replacement = replacement.str();

	/* possible macro to macro */
	if (i == 1)
		m->subrepl = repl;

	MRQ.push_back(MacroRecord(m, MacroRecord::Definition, m->definition));
}

void
MacroCallbacks::CompleteDefinition(Macro *m)
{
	MacroInfo	*MI = m->MI;
	Function	*frepl = NULL;

	if (m->component)
		return;

	if (m->subrepl && m->subrepl->function)
		m->function = true;

	if (m->sub.valid) {
		frepl = Model::lookup<Function>(FUNCTION_REG, m->sub.identifier);
		if (!frepl)
		    frepl = Model::lookup<Function>(FUNCTION_MACRO, m->sub.identifier);
		m->function = frepl != NULL;
	}

	if (!m->function && MI->isObjectLike()) {
		Data *mo = new Data(DATA_MACRO, m->id, Component::TULocal);
		mo->macro = new Model::MacroInfo;
		mo->definition = AST::Location(*SM, m->definition);
		mo->value = m->replacement;
		m->component = mo;

		Model::bind(mo, mo->definition.filename());

		mout(mo);
	} else {
		m->function = true;
		Function *mf = new Function(FUNCTION_MACRO, m->id, Component::TULocal);
		mf->macro = new Model::MacroInfo;
		mf->definition = AST::Location(*SM, m->definition);
		mf->arguments = m->args;
		m->component = mf;

		Model::bind(mf, mf->definition.filename());
		mout(mf);

		if (frepl) {
			mf->reference(frepl, Ref(REF_SUBSTITUTION));
		} else if (m->sub.valid) {
			MacroPostProcessQueue.push_back(MacroRecord(
				    m, MacroRecord::PostProcess, m->definition
			));
		}

		if (m->subrepl) {
			ReferenceDesc rd = Ref(REF_SUBSTITUTION);
			m->reference(m->subrepl, &rd);
		}
	}
	m->incomplete = false;

	MacroRefSet::iterator IT, END;
	for (IT = m->pendingrefs.begin(), END = m->pendingrefs.end(); IT != END; IT++) {
		Macro *r = IT->get();
		if (r->incomplete)
			CompleteDefinition(r);
		m->reference(r);
	}
	m->pendingrefs.clear();
}

void
MacroCallbacks::MacroUndefined(const Token &Identifier, const MacroInfo *MI)
{
	if (IS_BUILTIN_MACRO(SM, MI, MI->getDefinitionLoc()))
		return;

	IdentifierInfo	*II = Identifier.getIdentifierInfo();
	Macro		*m = mlookup(string(II->getNameStart(), II->getLength()))->current();

	m->undefinition = Identifier.getLocation();
	MRQ.push_back(MacroRecord(m, MacroRecord::Undefinition, m->undefinition));

	/* TODO */
#if 0
	m = Macro::variant(*m);
#endif
}

void
MacroCallbacks::MacroExpands(const Token &Id, const MacroInfo *MI)
{
	if (IS_BUILTIN_MACRO(SM, MI, MI->getDefinitionLoc()))
		return;

	IdentifierInfo	*II = Id.getIdentifierInfo();
	string		 name(II->getNameStart(), II->getLength());
	Macro		*m = mlookup(name)->current();
	SourceLocation	 loc = Id.getLocation();

	MRQ.push_back(MacroRecord(m, MacroRecord::Expansion, loc));
}

void
MacroCallbacks::MacroExpanded(const IdentifierInfo *II, const MacroInfo* MI,
			      const Token *TokStart, const unsigned NumTokens)
{
	if (IS_BUILTIN_MACRO(SM, MI, MI->getDefinitionLoc()))
		return;

	string		 name(II->getNameStart(), II->getLength());
	Macro		*m = mlookup(name)->current();

	// cerr << "<> EXPANDED: " << name << "\n";
	for (unsigned i = 0; i < NumTokens; i++) {
		const Token	*token = &TokStart[i];
		string		 spell;
		Macro		*sub = NULL;

		if (token->getKind() != tok::identifier)
			continue;

		spell = ConcatTokens(i, token, NumTokens);
#if 0
		cerr << "TOKEN: " << spell
		     << " i: " << i << " "
		     << token->getOriginMacro() << "\n";
#endif
		if (spell.size() == 0)
			continue;
		if (m->sub.valid && spell == m->sub.identifier)
			continue;

		sub = mlookup(spell, true);
		if (sub == NULL) {
			m->identifiers.insert(spell);
			continue;
		}
		if (sub->function) {
			/* chomp arguments */
			int plevel = 0;
			while ((i + 1) < NumTokens &&
			    (plevel += (&TokStart[i + 1])->getKind() == tok::l_paren)) {
				i++;
				// cerr << "> chomp: " << PP->getSpelling(TokStart[i]) << "\n";
				if ((&TokStart[i])->getKind() == tok::r_paren)
					plevel -= 1;
				if (plevel == 0)
					break;
			}
		}

#if 0		/* XXX: relies on my clang branch */
		const MacroInfo		*originm = token->getOriginMacro();
		const IdentifierInfo	*origini = token->getOriginMacroIdentifier();

		if (originm && origini) {
			string ospell(origini->getNameStart(), origini->getLength());
			sub = mlookup(ospell, true);
			if (sub && sub != m && !sub->function && sub->sub.valid) {
				// cerr << "EXPREF::MACRO: " << sub->id << "\n";
				m->reference(sub);
			}
		}
#endif
	}
}

string
MacroCallbacks::ConcatTokens(unsigned &position, const Token *token, const int maxTokens)
{
	ostringstream	result;
	int		index = 1;
	
	result << PP->getSpelling(token[0]);

	while ((int)position < (maxTokens - 2) &&
	    token[index].getKind() == tok::hashhash) {
		index++;

		if (token[index].getKind() == tok::identifier ||
		    token[index].getKind() == tok::numeric_constant)
			result << PP->getSpelling(token[index]);
		else
			return string();

		position += index;
	}

	return (result.str());
}

void
PostProcessMacros()
{
	MacroQueue::iterator	 MRI, MRE;

	MRE = MacroPostProcessQueue.end();
	while ((MRI = MacroPostProcessQueue.begin()) != MRE) {
		Macro		*m = MRI->macro;
		Function	*frepl = NULL;	

		frepl = Model::lookup<Function>(FUNCTION_REG, m->sub.identifier);
		if (!frepl)
		    frepl = Model::lookup<Function>(FUNCTION_MACRO, m->sub.identifier);

		if (frepl)
			m->component->reference(frepl, Ref(REF_SUBSTITUTION));

		MacroPostProcessQueue.pop_front();
	}

}

} /* namespace PP */ } /* namespace inquiry */
