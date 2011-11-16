/*	$SYSREVERSE: anontypedef.c,v 1.3 2011/05/05 00:28:10 proger Exp $	*/

#define VALSIZ	10

typedef struct {
	enum {
		ONE = 1,
		TWO,
		THREE
	}		 enumed;
	int		 val[VALSIZ];
	union {
		void		*dup;
	}		 child;
	void		*dup;
} parent_t;

typedef enum {		/* keep in sync with usbd_status_msgs */
	USBD_NORMAL_COMPLETION = 0, /* must be 0 */
	USBD_IN_PROGRESS,	/* 1 */
	/* errors */
	USBD_PENDING_REQUESTS,	/* 2 */
	USBD_NOT_STARTED,	/* 3 */
	USBD_INVAL,		/* 4 */
	USBD_NOMEM,		/* 5 */
	USBD_CANCELLED,		/* 6 */
	USBD_BAD_ADDRESS,	/* 7 */
	USBD_IN_USE,		/* 8 */
	USBD_NO_ADDR,		/* 9 */
	USBD_SET_ADDR_FAILED,	/* 10 */
	USBD_NO_POWER,		/* 11 */
	USBD_TOO_DEEP,		/* 12 */
	USBD_IOERROR,		/* 13 */
	USBD_NOT_CONFIGURED,	/* 14 */
	USBD_TIMEOUT,		/* 15 */
	USBD_SHORT_XFER,	/* 16 */
	USBD_STALLED,		/* 17 */
	USBD_INTERRUPTED,	/* 18 */

	USBD_ERROR_MAX		/* must be last */
} usbd_status;

usbd_status status;

int
getstatus(void)
{
	return (USBD_NO_POWER);
}
