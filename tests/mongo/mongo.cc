#include <sys/time.h>
#include <iostream>

#include <client/dbclient.h>

#define	MONGO_HOST	"127.0.0.1"
#define MONGO_PORT	"27017"
#define	MONGO_TEST_COLL	"test.inqmongo"

using namespace std;
using namespace mongo;


struct entry;
list<entry *>	AllEntries;

struct entry {
	OID		_oid;
	string		 name;
	list<entry *>	 friends;

	entry(string name)
	{
		this->name = name;
		this->_oid = OID::gen();

		AllEntries.push_back(this);
	}

	BSONObj
	bson()
	{
		BSONObjBuilder b;
#define nentries(l)	(sizeof(l) / sizeof(l[0]))
#define reflists(c)	c##_reflists
#define reftuple(v)	{ #v, &(v) }
#define entry_reflists	{ reftuple(friends) }
		struct {
			string		 lid;
			list<entry *>	*lref;
		} lists[] = reflists(entry);
		
		b.appendElements(BSON(
			"_id" << _oid
			<< "name" << name
		));
		
		for (unsigned i = 0; i < nentries(lists); i++) {
			list<entry *>::iterator IT = lists[i].lref->begin(),
				END = lists[i].lref->end();

			list<OID> oids;
			for (; IT != END; IT++)
				oids.push_back((*IT)->_oid);

			b.appendElements(BSON(lists[i].lid << oids));
		}

		return b.obj();
	};

	operator string()
	{
		return name;
	}
};

int
main(int argc, char **argv)
{
	struct timeval start, end, tdiff;
	DBClientConnection conn;
	string errmsg;

	string ns = MONGO_TEST_COLL;

	if (!conn.connect(string(MONGO_HOST) + ":" + string(MONGO_PORT), errmsg)) {
		cout << "couldn't connect: " << errmsg << "\n";
		exit(-1);
	}

	gettimeofday(&start, NULL);

	/*
	 * cleanup
	 */
	conn.dropCollection(ns);
	conn.remove(ns, BSONObj());
	assert(conn.findOne(ns, BSONObj()).isEmpty());

	/*
	 * add data
	 */
	auto_ptr<entry> o(new entry("john"));
	auto_ptr<entry> a(new entry("jack"));
	auto_ptr<entry> p(new entry("pollie"));
	o->friends.push_back(a.get());
	o->friends.push_back(p.get());

	for (list<entry *>::iterator IT = AllEntries.begin(), END = AllEntries.end(); IT != END; IT++) {
		entry *e = *IT;
		cout << "inserting data entries: " << static_cast<string>(*e) << "\n";
		conn.insert(ns, e->bson());
	}

	/*
	 * get mongod database entries
	 */
	list<string> l = conn.getDatabaseNames();
	cout << "databases: ";
	for (list<string>::iterator i = l.begin(), e = l.end(); i != e; i++) {
		cout << *i << " ";
	}
	cout << "\n";

	l = conn.getCollectionNames("test");
	cout << "collections in test: ";
	for (list<string>::iterator i = l.begin(), e = l.end(); i != e; i++) {
		cout << *i << " ";
	}
	cout << "\n";

	gettimeofday(&end, NULL);
	timersub(&end, &start, &tdiff);

	cout << "elapsed: " << tdiff.tv_sec << "s " << tdiff.tv_usec << "ms\n";

	return 0;
}
