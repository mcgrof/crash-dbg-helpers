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
	./get_dm_cell_idx disk170211 16 0xffffffffa06137a0

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

This will depend on what kernel you are using, in particular the size of
struct list_head can vary, even though it does not change often. Because of
this it is required as input. Which bucket a dm-target gets assigned to depends
on the name of the target using an internal algorithm, hash_str().
get_dm_cell_idx implements this in userspace and provided you also supply the
base address of the full array of the linked lists (_name_buckets) you can get 
the address you can use to start looking for the respective private dm data
structure.

For instance, if your disk name is disk170114, the size of struct list_head
is 16 bytes, and your base address of _name_buckets is 0xffffffffa06137a0 you
can use:

crash> struct list_head
struct list_head {
    struct list_head *next;
    struct list_head *prev;
}
SIZE: 16
crash> rd _name_buckets
ffffffffa06137a0:  ffff887d885a8ac0                    ..Z.}...

./get_dm_cell_idx disk170114 16 0xffffffffa06137a0
disk170114: Use _name_buckets[57] (0xffffffffa0613b30)

You can then use crash to verify the disk170114 is present:

crash> list -H -S hash_cell.name 0xffffffffa0613b30
ffff89bd07111280
  name = 0xffff89bd07667ee0  "disk170114"
ffff89bd080931c0
  name = 0xffff89bd0a661640  "disk170129"

Now to check the individual item struct hash_cell item disk170114 we know we
can use 0xffff89bd07111280 :

crash> struct hash_cell ffff89bd07111280
struct hash_cell {
  name_list = {
    next = 0xffff89bd080931c0,
    prev = 0xffffffffa0613b30 <_name_buckets+912>
  },
  uuid_list = {
    next = 0xffff89bd088e6510,
    prev = 0xffffffffa0613d80 <_uuid_buckets+480>
  },
  name = 0xffff89bd07667ee0 "disk170114",
  uuid = 0xffff89bd07111240 "mpath-360060e8007e574000030e57400000072",
  md = 0xffff89bd0713d800,
  new_map = 0x0
}

As reflected from the above results, the struct mapped_device can be obtained,
but to be clear:

crash> struct hash_cell.md ffff89bd07111280
  md = 0xffff89bd0713d800

We can use this to get map and flags members of struct mapped_device:

crash> struct -x mapped_device.map,flags 0xffff89bd0713d800
  map = 0xffff887d890e9400
  flags = 0x40

The map is a struct dm_table pointer, and this can be used to then obtain the
number of target devices and each corresponding struct dm_target. One issue is
that struct dm_table will not properly resolve on crash:

crash> struct dm_table
struct dm_table {
    int undefined__;
}
SIZE: 4

This is because it is a static symbol and its also referenced as a dummy
struct for RCU purposes in other files:

$ git grep -A 2 "struct dm_table {" drivers/md/
drivers/md/dm-ioctl.c:struct dm_table {
drivers/md/dm-ioctl.c-  int undefined__;
drivers/md/dm-ioctl.c-};
--
drivers/md/dm-table.c:struct dm_table {
drivers/md/dm-table.c-  struct mapped_device *md;
drivers/md/dm-table.c-  unsigned type;
--
drivers/md/dm.c:struct dm_table {
drivers/md/dm.c-        int undefined__;
drivers/md/dm.c-};

Because of this crash will not properly resolve and do struct unwinding
correctly when dealing with dm_table, we have to either do some hacks or
just work around over this struct.

We work around this. Assuming you have the following struct dm_table on your
kernel (size of members in paren, assuming x86_64), pay attention to
num_targets and the targets pointer:

#define MAX_DEPTH 16
struct dm_table {
	struct mapped_device *md; (8)
	unsigned type; (4)

	/* btree table */
	unsigned int depth;   (4)
	unsigned int counts[MAX_DEPTH]; /* in nodes */ (16*4 = 64)
	sector_t *index[MAX_DEPTH];  (8 * 16 = 128)

	unsigned int num_targets; (4) (at offset 208)
	unsigned int num_allocated; (4)
	sector_t *highs; (8)
	struct dm_target *targets;
	...
};

We then know we can get the num_targets at offset 208, and then 4 + 8 bytes
after the struct dm_targets. If the struct dm_table was at 0xffff887d890e9400
then:

Using python:
>>> import sys
>>> sys.stdout.write("0x%0x\n" % (0xffff887d890e9400 +  208))
0xffff887d890e94d0
>>> sys.stdout.write("0x%0x\n" % (0xffff887d890e9400 +  208 + 4 + 4 + 8))
0xffff887d890e94e0

We can now get the num_targets:

crash> rd -32 0xffff887d890e94d0
ffff887d890e94d0:  00000001                              ....

crash> rd 0xffff887d890e94e0
ffff887d890e94e0:  ffffc902f8f7f080                    ........

Now we know the only dm_target here is at 0xffffc902f8f7f080 and we can
inspect it:

crash> struct dm_target ffffc902f8f7f080
struct dm_target {
  table = 0xffff887d890e9400,
  type = 0xffffffffa07a4040 <multipath_target>,
  begin = 0,
  len = 2097152000,
  max_io_len = 0,
  num_flush_requests = 1,
  num_discard_requests = 1,
  private = 0xffff887d8c4fde00,
  error = 0xffffffffa060d82c "Unknown error",
  flush_supported = false,
  discards_supported = 0,
  split_discard_requests = 0,
  discard_zeroes_data_unsupported = 0
}

If you had a second target it should be at:

dm_target_idx(2) = address-of-targets + (2 * (sizeof(struct dm_target)))
dm_target_idx(3) = address-of-targets + (3 * (sizeof(struct dm_target)))

The private pointer in struct dm_target will vary depending on the type of dm
target, in this case the type tells us its a multipath target so we can inspect
its data structure:

crash> struct multipath 0xffff887d8c4fde00
struct multipath {
  list = {
    next = 0x0,
    prev = 0x0
  },
  ti = 0xffffc902f8f7f080,
  hw_handler_name = 0x0,
  hw_handler_params = 0x0,
... etc

get_dm_staus
------------

If you need to determine the status of the struct mapped_device, get the
struct mapped_device->flags and pass it to get_dm_status:

crash> struct -x mapped_device.flags 0xffff89bd06dc9000
  flags = 0x40

$ ./get_dm_status 0x40
DMF_MERGE_IS_OPTIONAL
