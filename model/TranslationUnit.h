/*	$SYSREVERSE: TranslationUnit.h,v 1.4 2010/12/19 19:33:49 proger Exp $	*/

#ifndef _SYSREV_INQUIRY_MODEL_TRANSLATIONUNIT_H_
#define _SYSREV_INQUIRY_MODEL_TRANSLATIONUNIT_H_

#include <stack>

namespace inquiry { namespace Model {

using std::string;
using std::list;
using std::set;
using std::stack;

extern ComponentSet	 Components;

struct File {
	string			 name;

	/* contains TULocal components */
	ComponentSet		 components;

	File() : components(component_ptr_less) {}
	File(string _name) : name(_name), components(component_ptr_less) {}

	bool operator<(const File &RHS) const {
		return name < RHS.name;
	}

	bool operator==(const File &RHS) const {
		return name == RHS.name;
	}

	static File		*create(string _name);
};

static inline bool
pointer_component_less(const Pointer<Component> &LHS, const Pointer<Component> &RHS)
{
	return component_ptr_less<Component>(LHS.get(), RHS.get());
}

struct TranslationUnit {
	ComponentPtrSet			*allComponents; /* lookup cache */
	File				*mainFile;

	bool operator<(const TranslationUnit &RHS) const {
		return mainFile < RHS.mainFile;
	}

	static TranslationUnit	*create(string mainfilename);
	File			*include(string filename);

	void enterContext() {
		allComponents = new ComponentPtrSet(pointer_component_less);
	}

	void leaveContext() {
		delete allComponents;
	}
};

extern set<File>		 AllFiles;
extern set<TranslationUnit *>	 TranslationUnits; /* pointers! */
extern TranslationUnit		*TU;	/* active TU */

static inline File *
bind(Component *object, const string &filename)
{
	static File	*_lastfile = NULL;

	if (object->flags & Component::TULocal) {
		if (!_lastfile || _lastfile->name != filename)
			_lastfile = File::create(filename);

		_lastfile->components.insert(object);
		TU->allComponents->insert(object);

		return _lastfile;
	} else {
		Components.insert(object);
		return NULL;
	}
}

#define LOOKUP_FULL	0
#define LOOKUP_LOCAL	1

template <typename T>
T *
lookup(T &dummy, int mode)
{
	set<Pointer<Component> >::iterator _IT;

	_IT = TU->allComponents->find(&dummy);
	if (_IT != TU->allComponents->end())
		return reinterpret_cast<T *>(_IT->get());
	else if (mode == LOOKUP_LOCAL)
		return NULL;

	set<Component *>::iterator IT;

	IT = Components.find(&dummy);
	if (IT != Components.end())
		return reinterpret_cast<T *>(*IT);

	return NULL;
}

template <typename T>
T *
lookup(unsigned short _type, string _id, int mode = LOOKUP_FULL)
{
	T	obj(_type, _id);

	return lookup<T>(obj, mode);
}

} /* namespace Model */ } /* namespace inquiry */

#endif /* _SYSREV_INQUIRY_MODEL_TRANSLATIONUNIT_H_ */
