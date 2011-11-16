/*	$SYSREVERSE: Document.cc,v 1.9 2010/12/22 10:45:58 proger Exp $	*/

#include <string>
#include <sstream>
#include <map>
#include <list>
#include <set>

#include <bson.h>
#include <mongo.h>

#include <types.h>

#include "Artifacts.h"
#include "Component.h"
#include "TranslationUnit.h"
#include "Document.h"

using std::cerr;
using std::map;
using std::list;

namespace inquiry { namespace Model {

DocumentMap DMap;

string	mongo_collection = "sysreverse.inqtest";
string	mongo_host = "127.0.0.1";
int	mongo_port = 27017;

static void
MapComponent(Component *c)
{
	if (c->isStructure())
		DMap[c] = new Document(reinterpret_cast<Structure *>(c));
	else if (c->isData())
		DMap[c] = new Document(reinterpret_cast<Data *>(c));
	else if (c->isFunction())
		DMap[c] = new Document(reinterpret_cast<Function *>(c));
}

void
MapComponents()
{
	set<File>::iterator		IT, END;
	set<Component *>::iterator	SI, SE;

	/* Top-level components */
	for (SI = Components.begin(), SE = Components.end(); SI != SE; SI++)
		MapComponent(*SI);

	/* TULocal components */
	for (IT = AllFiles.begin(), END = AllFiles.end(); IT != END; IT++)
		for (SI = IT->components.begin(), SE = IT->components.end(); SI != SE; SI++)
			MapComponent(*SI);
}

int
DumpDocuments()
{
	DocumentMap::iterator IT, END;
	mongo_connection conn;
	mongo_connection_options opts;
	bson key[1];
	bson_buffer key_buf[1];

	strlcpy(opts.host, mongo_host.c_str(), 255);
	opts.port = mongo_port;

	if (mongo_connect(&conn , &opts) != 0) {
		cerr << "mongo_connect failed\n";
		return (1);
	}

	for (IT = DMap.begin(), END = DMap.end(); IT != END; IT++) {
		Component *C = IT->first;
		Document *D = IT->second;

		cerr << "export mapping: " << C->id; 
		char oid[25] = {0};
		bson_oid_to_string(&D->_oid, oid);
		cerr << " -- " << oid << "\n";

		bson b = D->invokeGenerate();
		mongo_insert(&conn, mongo_collection.c_str(), &b);
		bson_destroy(&b);
	}

	bson_buffer_init(key_buf);
	bson_append_int(key_buf, "references.oid", 1);
	bson_from_buffer(key, key_buf);
	mongo_create_index(&conn, mongo_collection.c_str(), key,
	    0, NULL);

	mongo_destroy(&conn);

	return (0);
}

} /* namespace Model */ } /* namespace inquiry */
