void fun(void);

#define DATALEN	10

struct {
	int	data;
	union {
		int	ndata[DATALEN];
	}	next;
} anon;

enum {
	ONE,
	TWO,
	THREE
} anonenum;

enum { A, B, C };

static const struct {
	unsigned short uni;
	char ibm;
} unimappings[] = {
	{0x0192, 0x9f}, /* LATIN SMALL LETTER F WITH HOOK */
	{0x0393, 0xe2}, /* GREEK CAPITAL LETTER GAMMA */
};

void
fun(void)
{
	struct {
		union {
			char	c;
			int	i;
		};
	} funanon;
#if 0
	int a;
	a = 5;

	anon.data = a;
#else
	anon.data = 1;
#endif
}
