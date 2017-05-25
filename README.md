About
=====

This repository is intended to contain several utilities written to help work
with crash [0] for vmcore analysis.

[0] https://people.redhat.com/anderson/crash_whitepaper/

dm-target debugging
===================

Debugging dm-targets can be a non-trivial task. Here are some tools to help.

dm-target persistent names
===========================

Note that dm device numbers, ie, dm-13, are not persistent across reboots.
This does not mean that they cannot be, this is very system specific.
Persistent dm-names can be kept accross boots if DM_PERSISTENT_DEV_FLAG is
passed to the DM_DEV_CREATE_CMD, in this case the dm core code would
instantiate dm devices with DM_ANY_MINOR which means " give me the next
available minor number". This sequence can be seen in dm-ioctl.c:dev_create
function.

Currently DM_PERSISTENT_DEV_FLAG is not explicitly used on lvm2, userspace
tools set it only if the minor is set on the dm task created, for instance:

	lvcreate or lvchange --persistent y --major major --minor minor

Note that even if none of this is used a system *can* boot with the same
dm-target names ! So best is to just verify the dm-target and respective mount
point yourselves. How to do this is explained below.

Inspecting dm-target pending IO
===============================

If something is not going through you want to first inspect if any there is
any pending IO. You do this as follows, and in this example there is no pending
IO:

	crash> dev -d
	MAJOR GENDISK            NAME       REQUEST_QUEUE      TOTAL ASYNC  SYNC   DRV
	    8 ffff880fdae57800   sda        ffff880fdc2ad538       0     0     0     0
	    8 ffff880fdae58000   sdc        ffff880fde7b4e90       0     0     0     0
	    8 ffff880fdb5f3400   sde        ffff880fdc2ace50       0     0     0     0
	    8 ffff880fdb5b4c00   sdf        ffff880fdaef4ed0       0     0     0     0
	    8 ffff880fdb805c00   sdh        ffff880fdaef47e8       0     0     0     0
	    8 ffff880fdaeed400   sdg        ffff880fdae675f8       0     0     0     0
	    8 ffff880fdb5b8800   sdd        ffff880fde7b47a8       0     0     0     0
	  253 ffff881fe01a3400   dm-0       ffff881fddc20728       0     0     0     0
	    8 ffff880fdadb5800   sdb        ffff880fde7b5578       0     0     0     0
	    8 ffff880fdb5b4800   sdi        ffff880fda83f638       0     0     0     0
	    8 ffff880fdb5f2c00   sdj        ffff880fda83e868       0     0     0     0
	    8 ffff880fdc2f1000   sdk        ffff880fdc2ac768       0     0     0     0
	    8 ffff880fdae76000   sdl        ffff880fda83e180       0     0     0     0
	    8 ffff880fda7d0800   sdm        ffff880fdae66828       0     0     0     0
	    8 ffff880fda72e800   sdn        ffff880fdadc3678       0     0     0     0
	    8 ffff880fdafb2800   sdo        ffff880fda75f6b8       0     0     0     0
	  253 ffff881fdd833800   dm-1       ffff881fddc20e10       0     0     0     0
	    8 ffff880fda994000   sdp        ffff880fda75efd0       0     0     0     0
	  253 ffff880fdba02c00   dm-2       ffff880fdadc2f90       0     0     0     0
	  253 ffff881fdd82ec00   dm-3       ffff881fdce9f538       0     0     0     0
	  253 ffff880fda2fb800   dm-4       ffff880fdadc28a8       0     0     0     0
	  253 ffff881fdd812c00   dm-5       ffff881fdda5f578       0     0     0     0
	  253 ffff880fde1b2400   dm-6       ffff880fdae66f10       0     0     0     0
	  253 ffff881fddeee000   dm-7       ffff881fdce9e080       0     0     0     0
	  253 ffff881fddf3d400   dm-8       ffff881fdce9e768       0     0     0     0
	   11 ffff880fdb5a0c00   sr0        ffff881fddc214f8       0     0     0     0
	  253 ffff881fdecb5800   dm-9       ffff881fdda5e0c0       0     0     0     0
	  253 ffff881fdff36800   dm-10      ffff881fdee655b8       0     0     0     0
	  253 ffff881fdd811800   dm-11      ffff881fdd3f55f8       0     0     0     0
	  253 ffff881fdd829400   dm-12      ffff881fdee64ed0       0     0     0     0
	  253 ffff881fdd829c00   dm-13      ffff881fde84f638       0     0     0     0
	  253 ffff880d0f961800   dm-14      ffff880fdf1da280       0     0     0     0
	  253 ffff88198e0d4000   dm-15      ffff881fdee647e8       0     0     0     0
	  253 ffff8818733dd400   dm-16      ffff881fdee64100       0     0     0     0

If you run into this situation and are inspecting a soft lockup you should
first check the dm-status of the targets to verify they are not suspended.
Getting the dm-target can vary depending on what you are debugging, but the
crash dev -d list is a good start given you have access to the struct gendisk.

Let's assume you have a suspicion that dm-13 might be suspended. You can do
as follows:

	crash> dev -d | grep dm-13
	  253 ffff880fd8dd5400   dm-13      ffff880fde9bb778       0     0     0     0
	crash> struct gendisk.private_data ffff880fd8dd5400
	  private_data = 0xffff880fd8dd5800
	crash> struct -x mapped_device.pending,flags 0xffff880fd8dd5800
	  pending = {{
	      counter = 0x0
	    }, {
	      counter = 0x0
	    }},
	  flags = 0x43

You can now use the get_dm_staus tool provided in this package and documented
below.

Verifying dm-target mount points
================================

Since dm-target names can not be persistent its worth to always check their
mount points. You can do this by doing a reverse lookup on the struct
super_block first using the mount point. For example, say you want to inspect
xfs-path which is mounted on /some/ partition.

	crash> mount | grep p_accurevp
	ffff881fded2cec0 ffff881fde16cc00 xfs    /dev/mapper/vgSAN--some-disk-super-app /@/some/xfs-path
	crash> struct super_block.s_id ffff881fde16cc00
	  s_id = "dm-13\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"

This tells us /some/xfs-path is mounted using dm-13.

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

get_name_buckets
----------------

If you want to just display the address of all _name_bucket struct list_head
entries, assuming a struct list_head size of 16:

	$ ./get_name_buckets 0xffffffffa06137a0 16
	_name_buckets[0] = 0xffffffffa06137a0
	_name_buckets[1] = 0xffffffffa06137b0
	...
	_name_buckets[62] = 0xffffffffa0613b80
	_name_buckets[63] = 0xffffffffa0613b90

To verify you can inspect the struct list_head manually for both against the
symbol resolution used by crash. For instance let's verify _name_buckets[62]
maps to 0xffffffffa0613b80:

	crash> struct list_head 0xffffffffa0613b80
	struct list_head {
	  next = 0xffff89bd077c5bc0,
	  prev = 0xffff89bd097a3f00
	}

	crash> p _name_buckets[62]
	$10 = {
	  next = 0xffff89bd077c5bc0,
	  prev = 0xffff89bd097a3f00
	}

You can use these to inspect all dms in each bucket, say we want to inspect
further _name_buckets[62] which we have verified is at 0xffffffffa0613b80:

By name:

	crash> list -H -S hash_cell.name 0xffffffffa0613b80
	ffff89bd077c5bc0
	  name = 0xffff89bd07667480  "disk170113"
	ffff89bd097a3f00
	  name = 0xffff89bd0975f880  "disk170128"

By UUID:

	crash> list -H -S hash_cell.uuid 0xffffffffa0613b80
	ffff89bd077c5bc0
	  uuid = 0xffff89bd077c5b80  "mpath-360060e8007e574000030e57400000071"
	ffff89bd097a3f00
	  uuid = 0xffff89bd097a3ec0  "mpath-360060e8007e574000030e57400000080"

get_dm_staus
------------

If you need to determine the status of the struct mapped_device, get the
struct mapped_device->flags and pass it to get_dm_status:

	crash> struct -x mapped_device.flags 0xffff89bd06dc9000
	  flags = 0x40

	$ ./get_dm_status 0x40
	DMF_MERGE_IS_OPTIONAL
