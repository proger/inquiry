/*	$SYSREVERSE: types.h,v 1.2 2010/12/16 07:48:49 proger Exp $	*/

#ifndef _SYSREV_INQUIRY_TYPES_H_
#define _SYSREV_INQUIRY_TYPES_H_

template <typename T>
class Pointer {
	T	*p;
public:
	Pointer() {}
	Pointer(T *_p) : p(_p) {}
	virtual ~Pointer() {}

	virtual T *get() const { return p; }
	virtual T *operator->() { return p; }
	virtual bool operator<(const Pointer<T> &RHS) const { return p < RHS.p; }
	virtual bool operator==(const Pointer<T> &RHS) const { return p == RHS.p; }
};

#endif /* _SYSREV_INQUIRY_TYPES_H_ */
