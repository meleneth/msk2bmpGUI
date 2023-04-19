#pragma once
#include <SDL.h>
#include <filesystem>

#include "load_FRM_OpenGL.h"
#include "Load_Settings.h"
#include "shaders/shader_class.h"

//File info
struct LF {
    char Opened_File[MAX_PATH];
    char * c_name;
    char * extension;
    SDL_Surface* IMG_Surface = nullptr;
    SDL_Surface* PAL_Surface = nullptr;
    bool alpha = true;

    image_data img_data;
    image_data edit_data;

    img_type type;
    bool file_open_window = false;
    bool preview_tiles_window = false;
    bool show_image_render = false;
    bool edit_image_window = false;
    bool image_is_tileable = false;
    bool edit_MSK = false;
};

struct shader_info {
    float palette[768];
    Shader render_PAL_shader{ "shaders//passthru_shader.vert", "shaders//render_PAL.frag" };
    Shader render_FRM_shader{ "shaders//passthru_shader.vert", "shaders//render_FRM.frag" };
    mesh giant_triangle;
};

bool Load_Files(LF* F_Prop, image_data* img_data, struct user_info* user_info, shader_info* shaders);
bool File_Type_Check(LF* F_Prop, shader_info* shaders, image_data* img_data);
