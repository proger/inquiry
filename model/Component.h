/*	$SYSREVERSE: Component.h,v 1.50 2011/05/26 17:42:48 proger Exp $	*/

#ifndef _SYSREV_INQUIRY_MODEL_COMPONENT_H_
#define _SYSREV_INQUIRY_MODEL_COMPONENT_H_

namespace inquiry { namespace Model {

using std::string;
using std::list;
using std::set;
using std::pair;
using std::ostream;

typedef pair<string, string>	DeclPair;	/* (type, name) */
typedef list<DeclPair>		DeclPairList;

typedef pair<string, int>	ReferenceDesc;

inline ReferenceDesc
Ref(string kind, int arg = -1)
{
	return (make_pair(kind, arg));
}

class Component;

struct Reference {
	Component	*target;

	string		 refkind;
	int		 arg;
#define REF_BODYENC		"bodyenc"
#define REF_ARGUMENT		"argument"	/* arg: arg index */
#define REF_FIELD		"field"		/* arg: field index */
#define REF_FIELDACCESS		"fieldaccess"	/* arg: field index */
#define REF_ENUM		"enumval"
#define REF_INITIALIZER		"initializer"
#define REF_MACRODECL		"macrodecl"
#define REF_MACROHINT		"macrohint"
#define REF_SUBSTITUTION	"macrosubstitution"
#define REF_POINTER		"pointer"
#define REF_RETURN		"return"
#define REF_TYPE		"type"
#define REF_UNDERLAYER		"underlayer"
#define REF_INHERITANCE		"inheritance"

	Reference(Component *_target, string _kind, int _arg)
	    : target(_target), refkind(_kind), arg(_arg) {}

	bool operator<(const Reference &RHS) const {
		bool cmp;

		cmp = arg < RHS.arg;
		if (cmp == false)
			cmp = refkind < RHS.refkind;
		if (cmp == false)
			cmp = target < RHS.target;

		return cmp;
	}

	bool operator==(const Reference &RHS) const {
		return (refkind == RHS.refkind && arg == RHS.arg && target == RHS.target);
	}
};

#define COMTYPE(t)		(t ? (1 << (t - 1)) : 0)
#define TYPEDESCOFF(t)		ffs(t)
static const string TypeDescriptors[] = {
#define TYPE_UNKNOWN		0
	"unknown",

#define TYPE_STRUCT		1
#define TYPE_UNION		2
#define TYPE_ENUM		3
#define TYPE_TYPEDEF		4
	"struct", "union", "enum", "typedef",
#define COMPONENT_STRUCTURE	((1 << 4) - 1)

#define DATA_GLOBAL		5
#define DATA_ENUMVAL		6
#define DATA_MACRO		7
	"global", "enumval", "macro",
#define COMPONENT_DATA		(((1 << 7) - 1) & ~COMPONENT_STRUCTURE)

#define FUNCTION_REG		8
#define FUNCTION_MACRO		9
	"function", "macrofunction",
#define COMPONENT_FUNCTION	(((1 << 9) - 1) & ~COMPONENT_DATA & ~COMPONENT_STRUCTURE)
};

class Component {
public:
	unsigned short		 type;
	string			 id;

	set<Reference>		 references;

	set<Location>		 declarations;
	Location		 definition;

	Component		*parent;
	unsigned char		 flags;

	enum Flags {
		TULocal = 0x1, /* scoped to translation unit (e.g. static) */
	};

	Component() {}
	Component(unsigned short _type, string _id, unsigned char _flags = 0)
	    : id(_id), parent(NULL), flags(_flags) {
		type = COMTYPE(_type);
	}
	virtual ~Component() {}

	inline void reference(Component *object, ReferenceDesc refd) {
		Reference	 r(object, refd.first, refd.second);
		references.insert(r);
	}

	bool operator==(const Component &RHS) const {
		bool	feq = true;
		if (flags & TULocal)
			feq = (RHS.flags & TULocal)
			    ? definition == RHS.definition
			    : false;
		return (id == RHS.id && type == RHS.type && feq);
	}

	bool isStructure() { return (type & COMPONENT_STRUCTURE); }
	bool isData() { return (type & COMPONENT_DATA); }
	bool isFunction() { return (type & COMPONENT_FUNCTION); }

	virtual void dump(ostream &out);
};

class Structure : public Component {
public:
	DeclPairList		 fields;	/* for records */
	bool			 bodycomplete;

	Structure(unsigned short _type, string _id, unsigned char _flags = 0)
	    : Component(_type, _id, _flags), bodycomplete(false) {}

	virtual void dump(ostream &out);
};

struct MacroInfo {
	Location	 undefinition;
};

class Data : public Component {
public:
	string			 datatype;
	string			 value;

	MacroInfo		*macro;

	Data(unsigned short _type, string _id, unsigned char _flags = 0)
	    : Component(_type, _id, _flags), macro(NULL) {
		if (_type == DATA_MACRO)
		    flags |= TULocal;
	}
	virtual ~Data() { delete macro; }
};

class Function : public Component {
public:
	string			 returntype;
	DeclPairList		 arguments;
	MacroInfo		*macro;
	bool			 argscomplete;

	Function(unsigned short _type, string _id, unsigned char _flags = 0)
	    : Component(_type, _id, _flags), macro(NULL), argscomplete(false) {
		if (_type == FUNCTION_MACRO)
			flags |= TULocal;
	}
	virtual ~Function() { delete macro; }
};

template <typename T>
bool
component_ptr_less(const T *LHS, const T *RHS)
{
	if (LHS->type == RHS->type) {
		if (LHS->id == RHS->id) {
			if (LHS->definition.invalid() || RHS->definition.invalid())
				return false;
		}
		return LHS->id < RHS->id;
	}
	return LHS->type < RHS->type;
}

typedef set<Component *, bool (*)(const Component *, const Component *)> ComponentSet;
typedef set<Pointer<Component>, bool (*)(const Pointer<Component> &, const Pointer<Component> &)> ComponentPtrSet;

void	 DumpAll();

} /* namespace Model */ } /* namespace inquiry */

#endif /* _SYSREV_INQUIRY_MODEL_COMPONENT_H_ */
