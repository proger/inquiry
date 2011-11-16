#include "types.h"
#include "case01.h"

struct file;

int kqueue_read();
int kqueue_write();
int kqueue_ioctl();
int kqueue_poll();
int kqueue_kqfilter();
int kqueue_stat();
int kqueue_close();
int fun_fr(struct file *);

struct	fileops {
	int	(*fo_read)(void);
	int	(*fo_write)(void);
	int	(*fo_ioctl)(void);
	int	(*fo_poll)(void);
	int	(*fo_kqfilter)(void);
	int	(*fo_stat)(void);
	int	(*fo_close)(void);

	int	(*fileref)(struct file *);

	u_int16_t k;
	u_int16_t n;

	struct embedded {
		int 	(*a)(void);
		int	b;
	} emb;
};

#define KQUEUE_MAGIC	10
const int kqglobal = 5;

#if 1
struct fileops kqueueops = {
	.fo_read = kqueue_read,
	.fo_write = kqueue_write,
	.fo_ioctl = kqueue_ioctl,
	.fo_poll = kqueue_poll,
	.fo_kqfilter = kqueue_kqfilter,
	.fo_stat = kqueue_stat,
	.fo_close = kqueue_close,
	.fileref = fun_fr,
	//.n = kqglobal,
	.k = KQUEUE_MAGIC,
	.emb.a = kqueue_read,
};
#else
struct fileops kqueueops = {
	kqueue_read,
	kqueue_write,
	kqueue_ioctl,
	kqueue_poll,
	kqueue_kqfilter,
	kqueue_stat,
	kqueue_close,
	fun_fr,
	kqglobal,
	KQUEUE_MAGIC,
};
#endif

struct fileops bigops[] = {
	{ kqueue_read },
	{ 0, kqueue_write },
	{ 0, 0, kqueue_ioctl },
	{ 0 },
};

int boo = 100500;
