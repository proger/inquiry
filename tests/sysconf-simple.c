typedef unsigned long off_t;
typedef unsigned long paddr_t;
struct dev {};
struct proc {};
typedef struct dev dev_t;

#define      __CONCAT(x,y)   x ## y

#define	dev_decl(n,t)	__CONCAT(dev_type_,t)(__CONCAT(n,t))

int
polldummy(dev_t d, int i, struct proc *p)
{
	return (0);
}

#define	dev_type_poll(n)	int n(dev_t, int, struct proc *)
#define	dev_type_mmap(n)	paddr_t n(dev_t, off_t, int)

#define	cdev_decl(n) \
	dev_decl(n,poll); dev_decl(n,mmap);

#define biospoll polldummy
cdev_decl(bios);
