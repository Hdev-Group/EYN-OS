#ifndef SUBCOMMANDS_H
#define SUBCOMMANDS_H

#include "types.h"

// Read sub-command functions
void read_raw_cmd(string ch);
void read_md_cmd(string ch);
void read_image_cmd(string ch);

// Search sub-command functions
void search_size_cmd(string ch);
void search_type_cmd(string ch);
void search_empty_cmd(string ch);
void search_depth_cmd(string ch);

// Help sub-command functions
void help_write_cmd(string ch);
void help_search_cmd(string ch);
void help_fs_cmd(string ch);
void help_edit_cmd(string ch);
void help_system_cmd(string ch);
void help_game_cmd(string ch);
void help_dev_cmd(string ch);

// LS sub-command functions
void ls_tree_cmd(string ch);
void ls_size_cmd(string ch);
void ls_detail_cmd(string ch);

// Filesystem utility sub-command functions
void fsstat_cmd(string ch);
void cache_stats_cmd(string ch);
void cache_clear_cmd(string ch);
void cache_reset_cmd(string ch);
void blockmap_cmd(string ch);
void debug_superblock_cmd(string ch);
void debug_directory_cmd(string ch);

#endif // SUBCOMMANDS_H 