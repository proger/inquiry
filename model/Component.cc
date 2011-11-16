/*	$SYSREVERSE: Component.cc,v 1.19 2010/12/20 17:11:12 proger Exp $	*/

#include <iostream>
#include <sstream>
#include <string>
#include <bits/stl_pair.h>
#include <set>
#include <map>
#include <list>

#include <types.h>

#include "Artifacts.h"
#include "Component.h"
#include "TranslationUnit.h"

namespace inquiry { namespace Model {

using std::ostringstream;
using std::cerr;

void
Component::dump(ostream &out)
{
	out << "* " << TypeDescriptors[TYPEDESCOFF(this->type)] << " " << this->id << ":";
	out << " definition "
	    << (this->definition.filename().size() ? this->definition.filename() : "NONE");
	if (this->parent)
		out << " parent " << this->parent->id;
	if (this->flags)
		out << " flags " << (int)this->flags;
	out << "\n";
}

void
Structure::dump(ostream &out)
{
	DeclPairList::iterator	IT, END;

	Component::dump(out);
	for (IT = fields.begin(), END = fields.end(); IT != END; IT++) {
		DeclPair field = *IT;
		out << "    " << field.first << " " << field.second << "\n";
	}
}

void
DumpAll()
{
#define DUMPSET(l) do {									\
		bool		any = false;						\
		ostringstream	v;							\
		for (set<Reference>::iterator _IT = (l).begin(), _END = (l).end();	\
		     _IT != _END; _IT++) {						\
			any = true;							\
			Component *obj = (_IT)->target;					\
			v << "(" << obj->id << ", " << (_IT)->refkind << ") ";		\
		}									\
		if (any)								\
			cerr << "   " << #l << ": " << v.str() << "\n";			\
	} while (0)									\

	set<Component *>::iterator	SI, SE;

	for (SI = Components.begin(), SE = Components.end(); SI != SE; SI++) {
		Component	*obj = *SI;
		obj->dump(cerr);
		DUMPSET(obj->references);
	}

	set<File>::iterator	IT, END;

	for (IT = AllFiles.begin(), END = AllFiles.end(); IT != END; IT++) {
		SI = (IT)->components.begin();
		SE = (IT)->components.end();

		cerr << "=== file: " << (IT)->name << " ===\n";
		for (; SI != SE; SI++) {
			Component	*obj = *SI;
			obj->dump(cerr);
			DUMPSET(obj->references);
		}
	}
#undef DUMPSET
}

} /* namespace Model */ } /* namespaces inquiry */
