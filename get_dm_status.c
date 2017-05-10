#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define DMF_BLOCK_IO_FOR_SUSPEND 0
#define DMF_SUSPENDED 1
#define DMF_FROZEN 2
#define DMF_FREEING 3
#define DMF_DELETING 4
#define DMF_NOFLUSH_SUSPENDING 5
#define DMF_MERGE_IS_OPTIONAL 6

/* struct mapped_device->flags */
static void print_status(unsigned long flags)
{
	if (flags & (1 << DMF_BLOCK_IO_FOR_SUSPEND))
		printf("DMF_BLOCK_IO_FOR_SUSPEND\n");
	if (flags & (1 << DMF_SUSPENDED))
		printf("DMF_SUSPENDED\n");
	if (flags & (1 << DMF_FROZEN))
		printf("DMF_FROZEN\n");
	if (flags & (1 << DMF_FREEING))
		printf("DMF_FREEING\n");
	if (flags & (1 << DMF_DELETING))
		printf("DMF_DELETING\n");
	if (flags & (1 << DMF_NOFLUSH_SUSPENDING))
		printf("DMF_NOFLUSH_SUSPENDING\n");
	if (flags & (1 << DMF_MERGE_IS_OPTIONAL))
		printf("DMF_MERGE_IS_OPTIONAL\n");
}

void usage(void)
{
	printf("Usage: get_dm_status <value_mapped_device_flags>]\n");
	printf("Examples:\n\n");
	printf("\t./get_dm_status 0x40\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	const char *flags_str;
	unsigned long int flags;

	if (argc != 2)
		usage();

	flags_str= argv[1];
	flags = strtoul(flags_str, NULL, 16);
	print_status(flags);

	return 0;
}
