/*	$SYSREVERSE: Artifacts.cc,v 1.9 2010/12/27 16:50:54 proger Exp $	*/

#include <limits.h>
#include <stdlib.h>

#include <iostream>
#include <sstream>
#include <string>
#include <list>
#include <map>
#include <set>
#include <stack>
#include <bits/stl_pair.h>

#include <types.h>

#include "Artifacts.h"
#include "Component.h"
#include "TranslationUnit.h"

namespace inquiry { namespace Model {

using std::pair;

set<File>		 AllFiles;
set<TranslationUnit *>	 TranslationUnits;
ComponentSet		 Components(component_ptr_less);

TranslationUnit		*TU = NULL;	/* active TU */

File *
Location::file()
{
	if (!_file)
		_file = File::create(_filename);
	return _file;
}

void
Location::dump(ostream &out)
{
	if (this->line && this->column)
		out << this->file()->name << ":"
		    << this->line << ":"
		    << this->column;
}

File *
File::create(string _name)
{
	pair<set<File>::iterator, bool> insertion;
	char			 resolved[PATH_MAX] = {0};
	string			 realname;

	realpath(_name.c_str(), resolved);
	realname = resolved;

	File			 f(realname);
	set<File>::iterator	 IT = AllFiles.find(f);

	if (IT != AllFiles.end())
		return const_cast<File *>(&*IT);

	insertion = AllFiles.insert(f);
	return const_cast<File *>(&*insertion.first);
}

TranslationUnit *
TranslationUnit::create(string mainfilename)
{
	pair<set<TranslationUnit *>::iterator, bool> insertion;
	TranslationUnit		*tu = new TranslationUnit();

	insertion = TranslationUnits.insert(tu);
	if (!insertion.second) { /* insertion failed, have old element */
		delete tu;
		return *insertion.first;
	}

	tu->mainFile = File::create(mainfilename);

	return tu;
}

File *
TranslationUnit::include(string filename)
{
	File				*f = File::create(filename);
	set<Component *>::iterator	 IT, END, match;

	for (IT = f->components.begin(), END = f->components.end(); IT != END; IT++) {
		this->allComponents->insert(*IT);
	}

	return f;
}

} /* namespace Model */ } /* namespace inquiry */
