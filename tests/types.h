#ifndef _TYPES_H_
#define _TYPES_H_

#define __packed __attribute__((__packed__))

typedef	unsigned char		u_int8_t;
typedef	unsigned short		u_int16_t;
typedef	unsigned int		u_int32_t;
typedef	unsigned long long	u_int64_t;

typedef	long long		off_t;

typedef char *			caddr_t;

struct types_structure;

struct types_structure {
	int	 field0;
	char	 field1;
};

#endif
