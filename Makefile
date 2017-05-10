
all: get_dm_cell_idx \
     get_dm_status \
     get_name_buckets

get_dm_cell_idx: get_dm_cell_idx.c
	gcc -Wall -o get_dm_cell_idx get_dm_cell_idx.c

get_dm_status: get_dm_status.c
	gcc -Wall -o get_dm_status get_dm_status.c

get_name_buckets: get_name_buckets.c
	gcc -Wall -o get_name_buckets get_name_buckets.c

clean:
	rm -f get_dm_cell_idx get_dm_status get_name_buckets
