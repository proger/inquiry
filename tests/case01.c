/*	$SYSREVERSE: case01.c,v 1.24 2010/12/03 06:02:02 proger Exp $	*/

#define NULL (void *)0
#include "types.h"
#include "case01.h"

struct simple {
	int	x;
} const static unimappings = { 10 };
typedef struct simple simple_t;

typedef struct {
	simple_t	 simple;
	enum {
		ONE = 1,
		TWO,
		THREE
	}		 enumed;
	union {
		struct data0	*pd0;
		simple_t	*pd1;
		void		*dup;
	}		 data;
	int		 kind;
#define DATA_KIND_ZERO	0
#define DATA_KIND_ONE	1
	void		*dup;
} complex_t;

struct {
	int	 kind;
	char	*descr;
} static const descriptions[] = {
	{ DATA_KIND_ZERO,	"zero", },
	{ DATA_KIND_ONE,	"one",	},
	{ -1,			NULL, 	},
};

typedef struct name {
	union nm {	/* struct and union names are in the same namespace */
		int	a;
		int	b;
	}	 name;
	void	*p;
} name;

enum colors {
	GREEN,
	YELLOW,
	BLUE,
	WHITE = 10,
	BLACK
};

complex_t	*attach(struct simple *d);
int		 dummy(int);
static int	 dummy2(int);
typedef complex_t *(*attachfun_t)(struct simple *);

name n;
int global_a, global_b = 5;

int
main(int argc, char **argv)
{
	int i;
	complex_t d;
	struct internal {
		complex_t *p;
		struct other {
			void *x;
		} q;
	} internal;
	struct {
		struct internal *q;
	} anon = { &internal };
	attachfun_t fun;

	typedef struct internal internal_t;

	for (i = 0; i < argc; i++, attach(NULL)) {
#if 0
		_builtin_printf("arg: %s\n", argv[i]);
#else
		attach((struct simple *)descriptions[2].descr); /* o_O */
		global_b++;
#endif
	}

	i = global_b;
	d.kind = (i > 1) ? DATA_KIND_ONE : DATA_KIND_ZERO;

	internal.p = &d;
	DATA0_INITIALIZE(internal.p->data.pd0, NULL);

	dummy(dummy2(i));

	fun = attach;
	fun((struct simple *)&d);

	return (i);
}

complex_t *
attach(struct simple *d)
{
	complex_t	*ct;

	if (d == NULL)
		return (NULL);

	ct = (complex_t *)d;

	return (ct);
}

int
dummy(int arg)
{
	return arg;
}

static int
dummy2(int arg)
{
	return arg;
}
