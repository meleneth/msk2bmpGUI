#pragma once

#include "Load_Settings.h"
#include "town_map_tiles.h"

struct tile_name {
    char* name_ptr;
    int length;
    tile_name* next;
};

struct tile_name_arr {
    char*    name_ptr;
    uint32_t length;
    uint32_t next;           //points to array index of next viable name
};

bool append_TMAP_tiles_LST(user_info* usr_nfo, tt_arr_handle* handle, export_state* state);
tile_name_arr* make_name_list_arr(char* new_tiles_list);

char* save_NEW_FRM_tiles_LST(tt_arr_handle* handle, char* save_path, export_state* state);
char* load_LST_file(char* game_path, char* LST_path, char* LST_file);
char* append_FRM_tiles_LST(char* tiles_lst_path, tt_arr_handle* handle, export_state* state);
bool load_FRM_tiles_LST(user_info* usr_nfo, export_state* cur_state);
void set_false(export_state* cur_state);
char* check_FRM_LST_names(char* tiles_lst, tt_arr_handle* handle, export_state* state);
void append_FRM_tiles_POPUP(user_info* usr_nfo, tt_arr_handle* handle, export_state* state, bool auto_export);
