#define LIST_ENTRY(t, f)	struct t *f##_next
#define DLIST(n, k)		struct data##n a##k; \
				struct data##k a##n
#define DFUN(n)			int proc##n(struct data##n *)

struct data1 {
	void	*p;
	LIST_ENTRY(data1, d1);
};

struct data2 {
	int	data;
};

DLIST(1, 2);

#define declmany() DFUN(1); DFUN(2)

declmany();

inline int
a(int arg, int flags)
{
	return (arg | flags);
}

#define A 15

#define b a
#define c b
#define d c
#define e(a) d(a, 0)

int
call_flags(int arg, int flags)
{
	return (arg + flags);
}

#define call(arg)	call_flags(arg, 0)
#define _call		call_flags

int
wrapper(void)
{
	call(5);

	return 10;
}
