typedef unsigned long u_long;
typedef unsigned int u_int;
typedef unsigned long caddr_t;
typedef unsigned long paddr_t;
typedef unsigned long off_t;
struct dev {};
struct proc {};
struct uio;
struct tty;
struct knote;
typedef struct dev dev_t;

#define      __CONCAT(x,y)   x ## y

#define	dev_decl(n,t)	__CONCAT(dev_type_,t)(__CONCAT(n,t))
#define	dev_init(c,n,t) \
    	((c) > 0 ? __CONCAT(n,t) : (__CONCAT(dev_type_,t)((*))) enxio)

#define	dev_type_open(n)	int n(dev_t, int, int, struct proc *)
#define	dev_type_close(n)	int n(dev_t, int, int, struct proc *)
#define	dev_type_strategy(n)	void n(struct buf *)
#define	dev_type_ioctl(n) \
    	int n(dev_t, u_long, caddr_t, int, struct proc *)

struct cdevsw {
	int	(*d_open)(dev_t dev, int oflags, int devtype,
				     struct proc *p);
	int	(*d_close)(dev_t dev, int fflag, int devtype,
				     struct proc *);
	int	(*d_read)(dev_t dev, struct uio *uio, int ioflag);
	int	(*d_write)(dev_t dev, struct uio *uio, int ioflag);
	int	(*d_ioctl)(dev_t dev, u_long cmd, caddr_t data,
				     int fflag, struct proc *p);
	int	(*d_stop)(struct tty *tp, int rw);
	struct tty *
		(*d_tty)(dev_t dev);
	int	(*d_poll)(dev_t dev, int events, struct proc *p);
	paddr_t	(*d_mmap)(dev_t, off_t, int);
	u_int	d_type;
	u_int	d_flags;
	int	(*d_kqfilter)(dev_t dev, struct knote *kn);
};

int
polldummy(dev_t d, int i, struct proc *p)
{
	return (0);
}

/* cdevsw-specific types */
#define	dev_type_read(n)	int n(dev_t, struct uio *, int)
#define	dev_type_write(n)	int n(dev_t, struct uio *, int)
#define	dev_type_stop(n)	int n(struct tty *, int)
#define	dev_type_tty(n)		struct tty *n(dev_t)
#define	dev_type_poll(n)	int n(dev_t, int, struct proc *)
#define	dev_type_mmap(n)	paddr_t n(dev_t, off_t, int)
#define dev_type_kqfilter(n)	int n(dev_t, struct knote *)

#define	cdev_decl(n) \
	dev_decl(n,open); dev_decl(n,close); dev_decl(n,read); \
	dev_decl(n,write); dev_decl(n,ioctl); dev_decl(n,stop); \
	dev_decl(n,tty); dev_decl(n,poll); dev_decl(n,mmap); \
	dev_decl(n,kqfilter)

cdev_decl(pf);

#define biospoll polldummy
cdev_decl(bios);
