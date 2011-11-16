#include "types.h"

struct acpi_table_header {};
struct acpi_gas {};

struct acpi_hpet {
	struct acpi_table_header	hdr;
#define HPET_SIG	"HPET"
	u_int32_t	event_timer_block_id;
	struct acpi_gas	base_address;
	u_int8_t	hpet_number;
	u_int16_t	main_counter_min_clock_tick;
	u_int8_t	page_protection;
} __packed;

struct acpi_facs {
#define FACS_SIGNATURE	4
	u_int8_t	signature[FACS_SIGNATURE];
#define	FACS_SIG	"FACS"
	u_int32_t	length;
	u_int32_t	hardware_signature;
	u_int32_t	wakeup_vector;
	u_int32_t	global_lock;
#define	FACS_LOCK_PENDING	0x00000001
#define	FACS_LOCK_OWNED		0x00000002
	u_int32_t	flags;
#define	FACS_S4BIOS_F		0x00000001	/* S4BIOS_REQ supported */
	u_int64_t	x_wakeup_vector;
	u_int8_t	version;
#define FACS_RESERVED	31
	u_int8_t	reserved[FACS_RESERVED];
} __packed;

#undef FACS_SIGNATURE

struct acpi_facs_2 {
#define FACS_SIGNATURE	4
	u_int8_t	signature[FACS_SIGNATURE];
};
