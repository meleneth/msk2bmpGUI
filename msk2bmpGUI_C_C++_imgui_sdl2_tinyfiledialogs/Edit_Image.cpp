#include <stdio.h>
#include <stdlib.h>
#include <numeric>

#include "Edit_Image.h"
#include "imgui-docking/imgui.h"
#include "imgui-docking/imgui_internal.h"
#include "display_FRM_OpenGL.h"
#include "Load_Files.h"
#include "Zoom_Pan.h"

void Edit_Image(variables* My_Variables, LF* F_Prop, bool Palette_Update, uint8_t* Color_Pick) {
    //TODO: maybe pass the dithering choice through?

    image_data* edit_data = &F_Prop->edit_data;
    bool image_edited = false;
    float scale = edit_data->img_pos.new_zoom;
    int width   = edit_data->width;
    int height  = edit_data->height;
    ImVec2 size = ImVec2((float)(width * scale), (float)(height * scale));

    if (ImGui::Button("Clear All Changes...")) {

        int texture_size = width * height;
        uint8_t* clear = (uint8_t*)malloc(texture_size);
        memset(clear, 0, texture_size);

        glBindTexture(GL_TEXTURE_2D, edit_data->PAL_texture);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
            width, height,
            0, GL_RED, GL_UNSIGNED_BYTE, clear);

        free(clear);
    }


/////////////////////////////////////////////////////////////////////////////////
    //draw image with zoom and pan

    ImGuiWindow* window = ImGui::GetCurrentWindow();

    ImVec2 uv_min = My_Variables->uv_min;      // (0.0f,0.0f)
    ImVec2 uv_max = My_Variables->uv_max;      // (1.0f,1.0f)
    ImVec2* corner_pos = &edit_data->img_pos.corner_pos;
    ImVec2* bottom_corner = &edit_data->img_pos.bottom_corner;
    position* offset = &My_Variables->mouse_delta;

    ImVec2 window_size = ImGui::GetWindowSize();
    ImVec2 cursor_screen_pos = ImGui::GetCursorScreenPos();

    //calculate mouse offset and add to img_pos.offset
    panning(edit_data, *offset);

    
    edit_data->img_pos.offset.x = std::max((double)(window_size.x/2 - size.x), edit_data->img_pos.offset.x);
    edit_data->img_pos.offset.y = std::max((double)(window_size.y/2 - size.y), edit_data->img_pos.offset.y);
    edit_data->img_pos.offset.x = std::min((double)(window_size.x/2         ), edit_data->img_pos.offset.x);
    edit_data->img_pos.offset.y = std::min((double)(window_size.y/2         ), edit_data->img_pos.offset.y);
    


    //if (offset.x < window_size.x / 2 - size.s)
    //      { offset.x = window_size.x / 2 - size.x; }
    //else if (offset.x > window_size / 2)
    //      { offset.x = window_size / 2; }

    //image I'm trying to pan with
    window->DrawList->AddImage(
        (ImTextureID)edit_data->render_texture,
        *corner_pos, *bottom_corner,
        uv_min, uv_max,
        ImGui::GetColorU32(My_Variables->tint_col));


/////////////////////////////////////////////////////////////////////////////////



    float mouse_wheel = ImGui::GetIO().MouseWheel;
    if      (mouse_wheel > 0 && (ImGui::GetIO().KeyCtrl) && ImGui::IsWindowHovered()) {
        zoom_wrap(1.05, &F_Prop->edit_data);
    }
    else if (mouse_wheel < 0 && (ImGui::GetIO().KeyCtrl) && ImGui::IsWindowHovered()) {
        zoom_wrap(0.95, &F_Prop->edit_data);
    }

    if (ImGui::GetIO().MouseDown[0]) {
        image_edited = true;
        float x, y;
        x = ImGui::GetMousePos().x - corner_pos->x;
        y = ImGui::GetMousePos().y - corner_pos->y;

        if ((0 <= x && x <= size.x) && (0 <= y && y <= size.y)) {
            texture_paint(x/scale, y/scale, 10, 10, *Color_Pick, F_Prop->edit_data.PAL_texture);
        }
    }

    //Converts unpalettized image to texture for display
    if (Palette_Update || image_edited) {
        draw_PAL_to_framebuffer(My_Variables->palette,
            //&My_Variables->render_FRM_shader,
            &My_Variables->render_PAL_shader,
            &My_Variables->giant_triangle,
            &F_Prop->edit_data);
    }


}

SDL_Surface* Create_Map_Mask(SDL_Surface* image, GLuint* texture, bool* window)
{
    int width  = image->w;
    int height = image->h;

    //if (Map_Mask)
    //    { SDL_FreeSurface(Map_Mask); }
    SDL_Surface* Map_Mask = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);

    if (Map_Mask) {

        Uint32 color = SDL_MapRGBA(Map_Mask->format,
                                   0, 0, 0, 0);

        for (int i = 0; i < (width*height); i++)
        {
            ((Uint32*)Map_Mask->pixels)[i] = color; //rand() % 255;
            //uint8_t byte =
            //    (rand() % 2 << 0) |
            //    (rand() % 2 << 1) |
            //    (rand() % 2 << 2) |
            //    (rand() % 2 << 3) |
            //    (rand() % 2 << 4) |
            //    (rand() % 2 << 5) |
            //    (rand() % 2 << 6) |
            //    (rand() % 2 << 7);
            //((uint8_t*)MM_Surface->pixels)[i] = byte;
        }

        Image2Texture(Map_Mask,
            texture,
            window);
    }
    else {
        printf("Can't allocate surface for some reason...");
    }
    return Map_Mask;
}

//TODO: change input from My_Variables to F_Prop[]
void Edit_Map_Mask(LF* F_Prop, SDL_Event* event, bool* Palette_Update, ImVec2 Origin)
{
    int width  = F_Prop->image->w;
    int height = F_Prop->image->h;

    SDL_Surface* BB_Surface = F_Prop->Map_Mask;
    Uint32 white = SDL_MapRGB(F_Prop->Map_Mask->format,
                              255, 255, 255);

    ImVec2 MousePos = ImGui::GetMousePos();
    int x = (int)(MousePos.x - Origin.x);
    int y = (int)(MousePos.y - Origin.y);
    int pitch = BB_Surface->pitch;

    SDL_Rect rect;
    rect.x = x;
    rect.y = y;
    rect.h = 10;
    rect.w = 10;

    if (ImGui::IsMouseDown(event->button.button)) {

        if ((0 <= x && x <= width) && (0 <= y && y <= height)) {

            SDL_FillRect(F_Prop->Map_Mask, &rect, white);
            *Palette_Update = true;


            ///*TODO: This stuff didn't work, delete it when done                   */
            ///*TODO: two problems with using binary surface:
            ///       pixel addressing skips by 8 pixels at a time,
            ///       and SDL_FillRect() doesn't work                               */
            //for (int i = 0; i < 4; i++)
            //{
            //    uint8_t* where_i_want_to_draw = 
            //                &((uint8_t*)BB_Surface->pixels)[pitch*y + x/8 + i];
            //    ((uint8_t*)where_i_want_to_draw)[0] = 255;
            //}
            ///*OpenGl stuff didn't work either :                                   */
            //opengl_stuff();
        }
    }

    if (*Palette_Update && (F_Prop->type == FRM)) {
    ///re-copy Pal_Surface to Final_Render each time to allow 
    ///transparency through the mask surface painting
        Update_Palette(F_Prop, true);
    }
    else if (*Palette_Update && (F_Prop->type == MSK)) {
        Update_Palette(F_Prop, true);
    }
}

//bool blend == true = blend surfaces
void Update_Palette(struct LF* files, bool blend) {
    SDL_FreeSurface(files->Final_Render);
    if (files->type == MSK) {
        files->Final_Render =
            SDL_CreateRGBSurface(0, files->Map_Mask->w, files->Map_Mask->h, 32, 0, 0, 0, 0);
        SDL_BlitSurface(files->Map_Mask, NULL, files->Final_Render, NULL);
    }
    else {
        //Unpalettize image to new surface for display
        files->Final_Render = Unpalettize_Image(files->Pal_Surface);
    }
    if (blend) {
        CPU_Blend(    files->Map_Mask,
                      files->Final_Render);
        SDL_to_OpenGl(files->Final_Render,
                     &files->Optimized_Render_Texture);
    }
    else {
        //Image2Texture(files->Final_Render,
        //             &files->Optimized_Render_Texture,
        //             &files->edit_image_window);
        SDL_to_OpenGl(files->Final_Render,
            &files->Optimized_Render_Texture);
    }
}

//void Update_Palette2(SDL_Surface* surface, GLuint* texture, SDL_PixelFormat* pxlFMT) {
//    SDL_Surface* Temp_Surface;
//    //Force image to use the global palette instead of allowing SDL to use a copy
//    SDL_SetPixelFormatPalette(surface->format, pxlFMT->palette);
//    Temp_Surface = Unpalettize_Image(surface);
//    SDL_to_OpenGl(Temp_Surface, texture);
//
//    SDL_FreeSurface(Temp_Surface);
//}

void CPU_Blend(SDL_Surface* msk_surface, SDL_Surface* img_surface)
{
    int width  = msk_surface->w;
    int height = msk_surface->h;
    int pitch = msk_surface->pitch/4;

    Uint32 color_noAlpha = SDL_MapRGB(msk_surface->format,
                               255, 255, 255);
    Uint32 color_wAlpha = SDL_MapRGBA(img_surface->format,
                               255, 255, 255, 255);

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            int position = (pitch*i) + j;

            if (((Uint32*)msk_surface->pixels)[position] == color_noAlpha)
            {
                Uint32 pixel = ((Uint32*)img_surface->pixels)[position];
                uint8_t r, g, b, a;

                SDL_GetRGBA(pixel, img_surface->format, &r, &g, &b, &a);
                r = ((int)r + 255) / 2;
                g = ((int)g + 255) / 2;
                b = ((int)b + 255) / 2;
                a = ((int)a + 255) / 2;

                Uint32 color_wAlpha = SDL_MapRGBA(img_surface->format,
                                                  r, g, b, a);

                ((Uint32*)img_surface->pixels)[position] = color_wAlpha;
            }
        }
    }
}

void texture_paint(int x, int y, int brush_w, int brush_h, int value, unsigned int texture)
{
    int v = value;
    int size = brush_h * brush_w;
    uint8_t* color = (uint8_t*)malloc(size);
    memset(color, value, size);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, x-(brush_w/2), y-(brush_h/2), brush_w, brush_h,
        GL_RED, GL_UNSIGNED_BYTE,
        color);

    free(color);
}