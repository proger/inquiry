/*	$SYSREVERSE: Artifacts.h,v 1.18 2011/04/19 15:47:33 proger Exp $	*/

#ifndef _SYSREV_INQUIRY_MODEL_ARTIFACTS_H_
#define _SYSREV_INQUIRY_MODEL_ARTIFACTS_H_

namespace inquiry { namespace Model {

using std::ostream;
using std::ostringstream;
using std::string;
using std::list;
using std::set;

struct File;

class Location {
	string		 _filename;
	File		*_file;
public:
	unsigned int	 line;
	unsigned int	 column;

	Location() : _file(NULL), line(0), column(0) {}
	Location(string _name, unsigned _line = 0, unsigned _column = 0)
	    : _filename(_name), _file(NULL), line(_line), column(_column) {}
	virtual ~Location() {}

	File *file();
	virtual void dump(ostream &out); 

	const string &filename() const {
		return _filename;
	}

	bool invalid() const {
		return (!line || !column);
	}

	virtual string str() {
		ostringstream out;
		dump(out);
		return out.str();
	}

	virtual bool operator<(const Location &RHS) const {
		if (_file == RHS._file) {
			if (line == RHS.line)
				return column < RHS.column;
			else
				return line < RHS.line;
		} else
			return _file < RHS._file;
	}

	virtual bool operator==(const Location &RHS) const {
		return (invalid() == RHS.invalid() &&
		    _file == RHS._file && column == RHS.column &&
		    line == RHS.line);
	}
};

} /* namespace Model */ } /* namespace inquiry */

#endif /* _SYSREV_INQUIRY_MODEL_ARTIFACTS_H_ */
