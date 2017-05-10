#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define NUM_BUCKETS 64
#define MASK_BUCKETS (NUM_BUCKETS - 1)

void usage(void)
{
	printf("Usage: get_name_buckets <address of _name_buckets in hex>\n");
	printf("Examples:\n\n");
	printf("\t./get_name_buckets 0xffffffffa06137a0 <size-of-list-head>\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	const char *address_name_buckets_str, *str_sizeof_list_head;
	unsigned long int address, target_address;
	int sizeof_list_head;
	unsigned int idx;

	if (argc != 3)
		usage();

	address_name_buckets_str = argv[1];
	address = strtoul(address_name_buckets_str, NULL, 16);

	str_sizeof_list_head = argv[2];
	sizeof_list_head = atoi(str_sizeof_list_head);

	for (idx = 0; idx <  NUM_BUCKETS; idx++) {
		target_address = address + (sizeof_list_head * idx);
		printf("_name_buckets[%u] = 0x%0lx\n", idx, target_address);
	}

	return 0;
}
