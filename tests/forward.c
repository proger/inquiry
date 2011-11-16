struct proc;
static struct proc *p0;

#include "proc.h"

union data {
	int	 a;
};

int
main(int argc, char **argv)
{
	return (int)p0;
}
