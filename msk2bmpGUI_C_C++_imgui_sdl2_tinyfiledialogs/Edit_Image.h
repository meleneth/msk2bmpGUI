#pragma once
#include <SDL.h>
#include "Image2Texture.h"

void Edit_Image(variables* My_Variables, LF* F_Prop, bool Palette_Update, uint8_t* Color_Pick);
void Edit_Map_Mask(LF* F_Prop, SDL_Event* event, bool* Palette_Update, ImVec2 Origin);

SDL_Surface* Create_Map_Mask(SDL_Surface* image, GLuint* texture, bool* window);
void CPU_Blend(SDL_Surface* surface1, SDL_Surface* surface2);
void Update_Palette(struct LF* files, bool blend);
//void Update_Palette2(SDL_Surface* surface, GLuint* texture, SDL_PixelFormat* pxlFMT);

void texture_paint(int x, int y, int brush_w, int brush_h, int value, unsigned int texture);

