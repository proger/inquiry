/*	$SYSREVERSE: inheritance.c,v 1.1 2010/11/28 14:00:49 proger Exp $	*/

struct device {
	int		data;
	struct device	*parent;
};

struct driver_softc {
	struct device	devinfo;
};

int
driver_attach(struct device *dev, void *aux)
{
	struct driver_softc *sc;

	sc = (struct driver_softc *)dev;

	return !(struct device *)sc;
}
