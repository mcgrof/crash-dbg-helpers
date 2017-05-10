#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define NUM_BUCKETS 64
#define MASK_BUCKETS (NUM_BUCKETS - 1)

static unsigned int hash_str(const char *str)
{
	const unsigned int hash_mult = 2654435387U;
	unsigned int h = 0;

	while (*str)
		h = (h + (unsigned int) *str++) * hash_mult;

	return h & MASK_BUCKETS;
}

void usage(void)
{
	printf("Usage: get_dm_cell_idx <name> <sizeof-struct-hash-cell> [ <address of _name_buckets in hex>]\n");
	printf("Examples:\n\n");
	printf("\t./get_dm_cell_idx disk170211\n");
	printf("\t./get_dm_cell_idx disk170211 0xffffffffa06137a0\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	const char *str;
	const char *str_sizeof_list_head;
	const char *address_name_buckets_str;
	unsigned long int address, target_address;
	int sizeof_list_head;
	unsigned int idx;

	if (argc > 4 || argc < 3)
		usage();

	str = argv[1];
	idx = hash_str(str);

	str_sizeof_list_head = argv[2];
	sizeof_list_head = atoi(str_sizeof_list_head);
	printf("Size of hash cell: %d\n", sizeof_hash_cell);

	if (argc == 3)
		printf("%s: Use _name_buckets[%u]\n", str, idx);
	else if (argc == 4) {
		address_name_buckets_str = argv[3];
		address = strtoul(address_name_buckets_str, NULL, 16);
		target_address = address + (sizeof_list_head * idx);
		printf("%s: Use _name_buckets[%u] (0x%0lx)\n", str, idx, target_address);
	}

	return 0;
}
