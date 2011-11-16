struct point {
	int x;
	int y;
};

struct size {
	int w;
	int h;
};

static const struct {
	struct point	point;
	struct size	size;
} rect = {
	.point = {5, 6},
	.size = {10, 20},
};
