typedef struct {
	int	val;
} goo_t;

#define Static

Static const goo_t goo = {
	.val = 5,
};

Static const goo_t gooplain = {
	6
};
