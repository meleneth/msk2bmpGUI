#pragma once
#include <filesystem>
#include "platform_io.h"

#define MAX_KEY  32

enum export_auto {
    not_set  = 0,
    auto_all = 1,
    manual   = 2,
};

enum {
    CANCEL = 0,
    YES    = 1,
    NO     = 2,
};


struct fo2_files {
    char* FRM_TILES_LST = NULL;
    char* PRO_TILES_LST = NULL;
    char* PRO_TILE_MSG  = NULL;
    char* WORLDMAP_TXT  = NULL;
};

struct user_info {
    char default_save_path[MAX_PATH];   //change to last_save_path?
    char default_game_path[MAX_PATH];
    char default_load_path[MAX_PATH];   //change to last_load_path?
    char* exe_directory = NULL;

    fo2_files game_files;

    int  auto_export = not_set;           // 0=not set, 1=auto all the way, 2=manual
    bool save_full_MSK_warning;
    bool show_image_stats;          //TODO: remove, replace with window specific bool
    bool create_new_LST;
    size_t length;
};

enum img_type {
    UNK   = -1,
    MSK   =  0,
    FRM   =  1,
    FR0   =  2,
    FRx   =  3,
    TILE  =  3,
    OTHER =  4,
};

void Load_Config    (struct user_info *user_info, char* exe_path);
void write_cfg_file (struct user_info* user_info, char* exe_path);

void parse_data     (char *file_data, size_t size, struct user_info *user_info);
void parse_key      (char *file_data, size_t size, struct config_data *config_data);
void parse_comment  (char *file_data, size_t size, struct config_data *config_data);
void parse_value    (char *file_data, size_t size, struct config_data *config_data, struct user_info *user_info);
void store_config_info(struct config_data *config_data, struct user_info *user_info);
