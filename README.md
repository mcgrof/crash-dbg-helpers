About
=====

This repository is intended to contain several utilities written to help work
with crash [0] for vmcore analysis.

[0] https://people.redhat.com/anderson/crash_whitepaper/

dm-target debugging
===================

Debugging dm-targets can be a non-trivial task. Here are some tools to help.

get_dm_cell_idx
----------------

## Compiling get_dm_cell_idx
	make

## Using get_dm_cell_idx:
	./get_dm_cell_idx disk170211
	./get_dm_cell_idx disk170211 0xffffffffa06137a0

Where 0xffffffffa06137a0 is the base address of _name_buckets (which you can
obtain using rd _name_buckets, the address is the value on the left hand side):

crash> rd _name_buckets
ffffffffa06137a0:  ffff887d885a8ac0                    ..Z.}...

# Purpose

If you are debugging dm-targets you may want to get their target's respective
private data structure. For instance for dm-mpath (multipath) targets the
struct dm_target->private points to a struct multipath. To get the struct
multipath for each target is actually not quite trivial and get_dm_cell_idx
should help you on your way.

Each dm_target is tucked away in an array under struct dm_table. Of interest:

struct dm_table {
	...
	unsigned int num_targets;
	...
	struct dm_target *targets;
	...
};

The struct dm_table in turn can be reached via the struct mapped_device:

struct mapped_device {
	...
	struct dm_table *map;
	...
};

The struct mapped_device in turn is referred to on the struct hash_cell:

struct hash_cell {
	struct list_head name_list;
	struct list_head uuid_list;

	char *name;
	char *uuid;
	struct mapped_device *md;
	struct dm_table *new_map;
};

The struct hash_cell are itemized linked list items tucked into a linked list.
As per drivers/md/dm-ioctl.c there are NUM_BUCKETS (64) linked lists (buckets):

...
#define NUM_BUCKETS 64
#define MASK_BUCKETS (NUM_BUCKETS - 1)
static struct list_head _name_buckets[NUM_BUCKETS];
static struct list_head _uuid_buckets[NUM_BUCKETS];
...

On 64-bit systems each linked list is separated by 8 bytes. Which bucket a
dm-target gets assigned to depends on the name of the target using an internal
algorithm, hash_str(). get_dm_cell_idx implements this in userspace and
provided you also supply the base address of the full array of the linked lists
(_name_buckets) you can get the address you can use to start looking for the
respective private dm data structure.
