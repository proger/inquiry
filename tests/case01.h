#ifndef _CASE01_H_
#define _CASE01_H_

struct data1;

struct data0 {
	struct data1	*ptr;
};

#define DATA0_INITIALIZE(d0, p) 	do {		\
		(d0)->ptr = (p);			\
	} while (0)

extern int boo;

typedef int boo_t;
typedef boo_t boot_;

#endif /* _CASE01_H_ */
