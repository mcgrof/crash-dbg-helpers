
all: get_dm_cell_idx

get_dm_cell_idx: get_dm_cell_idx.c
	gcc -Wall -o get_dm_cell_idx get_dm_cell_idx.c
