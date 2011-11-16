/*	$SYSREVERSE: Document.h,v 1.15 2011/05/15 09:55:58 proger Exp $	*/

#ifndef _SYSREV_INQUIRY_MODEL_DOCUMENT_H_
#define _SYSREV_INQUIRY_MODEL_DOCUMENT_H_

/*
 * Mongo Document mapping proxies.
 */

#include <iostream>
#include <string>
#include <list>
#include <map>
#include <bits/stl_pair.h>

#include <bson.h>

#include "Component.h"

using std::string;
using std::list;
using std::ostringstream;
using std::map;

namespace inquiry { namespace Model {

class Document;
typedef map<Component *, Document *> DocumentMap;

extern DocumentMap	 DMap;

extern string		 mongo_collection;
extern string		 mongo_host;
extern int		 mongo_port;

class Document {
public:
	bson_oid_t	 _oid;

	union {
		Structure	*struc;
		Data		*data;
		Function	*func;
	}		 model;

	bson	(Document::*generate)(void);
	bson	invokeGenerate() { return (this->*generate)(); }

#define constructor(type, modelref)				\
	explicit Document(type *t) {				\
		this->model.modelref = t;			\
		this->generate = &Document::generate##type;	\
		bson_oid_gen(&this->_oid);			\
	}

	constructor(Structure, struc);
	constructor(Data, data);
	constructor(Function, func);
#undef constructor

/* ic -- iterable container */
#define bson_append_ic(ic, buf, name, type, l, method)	do {			\
		bson_buffer *arr = bson_append_start_array(buf, name);		\
		ic<type>::iterator IT, END;					\
		unsigned i = 0;							\
		for (IT = (l).begin(), END = (l).end();				\
		     IT != END; IT++, i++) {					\
			type o = *IT;						\
			method(arr, i, o);					\
		}								\
		bson_append_finish_object(arr);					\
	} while (0)

#define bson_append_list(buf, name, type, l, method) 				\
		bson_append_ic(list, buf, name, type, l, method)

#define bson_append_set(buf, name, type, l, method) 				\
		bson_append_ic(set, buf, name, type, l, method)

	static void
	bson_append_location(bson_buffer *arr, int i, Location &obj) {
		ostringstream n;
		n << i;

		bson_append_string(arr, n.str().c_str(), obj.str().c_str());
	}

	static void
	bson_append_declpair(bson_buffer *arr, int i, DeclPair &dp) {
		ostringstream n;
		n << i;

		bson_buffer *obj = bson_append_start_object(arr, n.str().c_str());
		bson_append_string(obj, "type", dp.first.c_str());
		bson_append_string(obj, "name", dp.second.c_str());
		bson_append_finish_object(obj);
	}

	static void
	bson_append_ref(bson_buffer *arr, int i, Reference &ref) {
		ostringstream n;
		n << i;

		bson_buffer *robj = bson_append_start_object(arr, n.str().c_str());
		bson_append_oid(robj, "oid", &DMap[ref.target]->_oid);
		bson_append_string(robj, "refkind", ref.refkind.c_str());
		bson_append_int(robj, "arg", ref.arg);
		bson_append_finish_object(robj);
	}

	bson_buffer generateComponent() {
		Component *c = model.struc;
		bson_buffer buf;

		bson_buffer_init(&buf);
		bson_append_oid(&buf, "_id", &_oid);

		bson_append_string(&buf, "type", TypeDescriptors[TYPEDESCOFF(c->type)].c_str());
		bson_append_string(&buf, "id", c->id.c_str());
		bson_append_string(&buf, "definition", c->definition.str().c_str());
		bson_append_int(&buf, "flags", c->flags);

		if (c->parent)
			bson_append_oid(&buf, "parent", &DMap[c->parent]->_oid);

		bson_append_set(&buf, "declarations", Location,
		    c->declarations, bson_append_location);

		/* MapComponents() must be called before this */
		bson_append_set(&buf, "references", Reference, c->references, bson_append_ref);

		return (buf);
	}

#define return_bson_from_buf(buf) do { bson _b; bson_from_buffer(&_b, buf); return (_b); } while (0)

	bson generateStructure() {
		Structure *s = model.struc;
		bson_buffer buf = generateComponent();

		bson_append_list(&buf, "fields", DeclPair, s->fields, bson_append_declpair);

		return_bson_from_buf(&buf);
	}

	bson generateData() {
		Data *d = model.data;
		bson_buffer buf = generateComponent();

		bson_append_string(&buf, "datatype", d->datatype.c_str());
		bson_append_string(&buf, "value", d->value.c_str());
		if (d->macro)
			bson_append_string(&buf, "undefinition",
			    d->macro->undefinition.str().c_str());

		return_bson_from_buf(&buf);
	}

	bson generateFunction() {
		Function *f = model.func;
		bson_buffer buf = generateComponent();

		bson_append_string(&buf, "returntype", f->returntype.c_str());
		bson_append_list(&buf, "arguments", DeclPair, f->arguments,
		     bson_append_declpair);
		if (f->macro)
			bson_append_string(&buf, "undefinition",
			    f->macro->undefinition.str().c_str());

		return_bson_from_buf(&buf);
	}
};

void	 MapComponents();
int	 DumpDocuments();

} /* namespace Model */ } /* namespace inquiry */

#endif /* _SYSREV_INQUIRY_MODEL_DOCUMENT_H_ */
