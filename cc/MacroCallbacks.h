/*	$SYSREVERSE: MacroCallbacks.h,v 1.14 2011/01/10 15:03:40 proger Exp $	*/

#ifndef _SYSREV_INQUIRY_PP_MACROCALLBACKS_H_
#define _SYSREV_INQUIRY_PP_MACROCALLBACKS_H_

#include <clang/Lex/MacroInfo.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/PPCallbacks.h>

namespace inquiry { namespace PP {

using std::string;
using std::list;
using std::set;
using std::map;
using std::ostringstream;
using namespace clang;

using Model::DeclPairList;
using Model::ReferenceDesc;
using Model::Data;
using Model::Function;
using Model::Location;
class Macro;
struct MacroRecord;

typedef map<string, Macro *>	MacroObjMap;
typedef set<Pointer<Macro> >	MacroRefSet;
typedef list<MacroRecord>	MacroQueue;

extern MacroQueue	 MRQ;

Macro			*mlookup(string _id, bool silent = false);

class MacroCallbacks : public PPCallbacks {
public:
	void Initialize(SourceManager &smref) {
		SM = &smref;
	}

	void		 CompleteDefinition(Macro *m);

	SourceManager	*SM;
	Preprocessor	*PP;

protected:
	virtual void	 FileChanged(SourceLocation Loc, FileChangeReason Reason,
				SrcMgr::CharacteristicKind FileType);

	virtual void	 MacroDefined(const Token &Identifier, const MacroInfo *MI);
	virtual void	 MacroUndefined(const Token &Identifier, const MacroInfo *MI);

	virtual void	 MacroExpands(const Token &Id, const MacroInfo *MI);
	virtual void	 MacroExpanded(const IdentifierInfo *II, const MacroInfo* MI,
				const Token *TokStart, const unsigned NumTokens);

	virtual void	 FileSkipped(const FileEntry &ParentFile,
				const Token &FilenameTok,
				SrcMgr::CharacteristicKind FileType) {}
	virtual void	 EndOfMainFile() {}

	string		 ConcatTokens(unsigned &position, const Token *token,
	    			const int maxTokens);
};

class Macro {
public:
	explicit Macro(string _id) : id(_id), component(NULL), incomplete(true),
		subrepl(NULL), function(false), variant_prev(NULL), variant_last(this) {
		sub.valid = false;
	}

	virtual	~Macro() {
		delete variant_prev;
	}

	Macro *current() {
		return variant_last;
	}

	bool operator==(const Macro &RHS) const {
		return definition == RHS.definition;
	}

	bool operator<(const Macro &RHS) const {
		return definition < RHS.definition;
	}

	void reference(Macro *RHS, ReferenceDesc *rd = NULL);

	string			 id;
	Model::Component	*component;
	bool			 incomplete;

	SourceLocation		 definition;
	SourceLocation		 undefinition;
	list<SourceLocation>	 expansions;

	struct {
		bool	valid;
		string	identifier;
	}			 sub;
	Macro			*subrepl;
	set<string>		 identifiers;

	string			 replacement;

	bool 			 function;
	DeclPairList		 args;

protected:
	static Macro *variant(Macro &orig) {
		Macro	*nm = new Macro(orig);

		orig.variant_last = nm->variant_last = nm;
		nm->variant_prev = &orig;

		return nm;
	}

	friend class MacroCallbacks;
	clang::MacroInfo	*MI;
	Macro			*variant_prev;
	Macro			*variant_last;
	MacroRefSet		 pendingrefs;
};

struct MacroRecord {
	enum RecordType {
		Definition,
		Undefinition,
		Expansion,
		PostProcess,
	};

	Macro		*macro;	
	RecordType	 type;
	SourceLocation	 location;

	MacroRecord(Macro *m, RecordType t, SourceLocation sl)
	    : macro(m), type(t), location(sl) {}
};

void	PostProcessMacros();

} /* namespace PP */ } /* namespace inquiry */

#endif /* _SYSREV_INQUIRY_PP_MACROCALLBACKS_H_ */
