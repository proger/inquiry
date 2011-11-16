#define NULL (void *)0

typedef int sometype_t;

struct pointers {
	void	*voidp;

	void 	(*coolfunp)(int);
	void 	(*coolfunpnarg)(int arg);

	sometype_t	fieldn;

	union {
		int	u1;
		char	u2;
	} _;

	struct embedded {
		int	 a;
	}	 emb;
};

int
f(struct pointers *x)
{
	struct embedded *emb = &x->emb;

	x->voidp = NULL;
	x->coolfunp = NULL;
	x->coolfunpnarg = NULL;

	return (emb->a);
}
