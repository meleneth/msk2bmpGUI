#define _CRTDBG_MAP_ALLOC
//#define SDL_MAIN_HANDLED

#define SET_CRT_DEBUG_FIELD(a) \
    _CrtSetDbgFlag((a) | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG))
#define CLEAR_CRT_DEBUG_FIELD(a) \
    _CrtSetDbgFlag(~(a) & _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG))

// Dear ImGui: standalone example application for SDL2 + OpenGL
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs


// ImGui header files
#include "imgui-docking/imgui.h"
#include "imgui-docking/imgui_impl_sdl.h"
#include "imgui-docking/imgui_impl_opengl3.h"
#include "imgui-docking/imgui_internal.h"

#include <stdio.h>
#include <SDL.h>
#include <SDL_blendmode.h>
#include <Windows.h>
#include <glad/glad.h>

#include <fstream>
#include <sstream>
#include <iostream>

// My header files
#include "Load_Files.h"
#include "FRM_Convert.h"
#include "Save_Files.h"
#include "Image2Texture.h"
#include "Load_Settings.h"
#include "FRM_Animate.h"
#include "Edit_Image.h"
#include "Shader_Stuff.h"

// Our state
struct variables My_Variables = {};
struct user_info user_info;

// Function declarations
void Show_Preview_Window(variables *My_Variables, int counter, SDL_Event* event); //, SDL_Renderer* renderer);
void Preview_Tiles_Window(variables *My_Variables,
    ImVec2 *Top_Left, ImVec2 *Bottom_Right, ImVec2 *Origin,
    int *max_box_x, int *max_box_y, int counter);
void Show_Image_Render(variables *My_Variables, struct user_info* user_info, int counter);
void Edit_Image_Window(variables *My_Variables, struct user_info* user_info, int counter, SDL_Event* event);

void Show_Palette_Window(struct variables *My_Variables, int counter);

void SDL_to_OpenGl(SDL_Surface *surface, GLuint *Optimized_Texture);
void Prep_Image(LF* F_Prop, SDL_PixelFormat* pxlFMT_FO_Pal, bool color_match, bool* preview_type);

static void ShowMainMenuBar(int* counter);
void Open_Files(struct user_info* user_info, int* counter, SDL_PixelFormat* pxlFMT);
void Set_Default_Path(struct user_info* user_info);
void crash_detector();




//int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
//    PSTR lpCmdLine, INT nCmdShow)
//{
//    return main(0, NULL);
//}

// Main code
int main(int, char**)
{
    //bug checking code
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);

    SET_CRT_DEBUG_FIELD(_CRTDBG_ALLOC_MEM_DF);
    SET_CRT_DEBUG_FIELD(_CRTDBG_CHECK_ALWAYS_DF);
    SET_CRT_DEBUG_FIELD(_CRTDBG_LEAK_CHECK_DF);
    CLEAR_CRT_DEBUG_FIELD(_CRTDBG_CHECK_CRT_DF);
    //end of bug checking code

    Load_Config(&user_info);
    //My_Variables.PaletteColors = loadPalette("file name for palette here");
    My_Variables.pxlFMT_FO_Pal = loadPalette("file name for palette here");

    // Setup SDL
    // (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
    // depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to latest version of SDL is recommended!)
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }
    SDL_GL_LoadLibrary(NULL);

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 330 core";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("Q's Crappy Fallout Image Editor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        printf("Glad Loader failed?..."); // %s", SDL_GetVideoDriver);
        exit(-1);
    }
    else
    {
        printf("Vendor: %s\n", glGetString(GL_VENDOR));
        printf("Renderer: %s\n", glGetString(GL_RENDERER));
        printf("Version: %s\n", glGetString(GL_VERSION));
        printf("GLSL Version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    }

    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    //Init Framebuffer stuff so I can use shaders

    init_framebuffer(&My_Variables.F_Prop[counter]->palette_buffer,
                     &My_Variables.F_Prop[counter]->palette_texture,
                     800, 600);
    mesh giant_triangle = load_giant_triangle();
    //Shader stuff
    Shader color_cycle("shaders\\zoom_shader.vert", "shaders\\color_cycle_1D.frag");

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    // used to reset the default layout back to original
    bool firstframe = true;

    // Main loop
    bool done = false;
    while (!done)
    {
    //-----------------------------------------------------------------------------------------
        //OpenGL stuff
        shader_setup(giant_triangle,
                    (int)io.DisplaySize.x, (int)io.DisplaySize.y,
                     My_Variables.F_Prop[counter]->palette_texture,
                     My_Variables.F_Prop[counter]->palette_buffer,
                     color_cycle);



    //-----------------------------------------------------------------------------------------


        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // Store these variables at frame start for cycling the palette colors
        My_Variables.CurrentTime    = clock();
        My_Variables.Palette_Update = false;

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        bool show_demo_window = false;
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        //ImGuiViewport* viewport = ImGui::GetMainViewport();
        bool * t = NULL;
        bool r = true;
        t = &r;
        ImGuiID dockspace_id = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
        ImGuiID dock_id_right = 0;
        static int counter = 0;
        ShowMainMenuBar(&counter);

        if (firstframe) {
            firstframe = false;
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace); // Add empty node
            ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->WorkSize);

            ImGuiID dock_main_id = dockspace_id; // This variable will track the docking node.
            ImGuiID dock_id_left = ImGui::DockBuilderSplitNode(       dock_main_id, ImGuiDir_Left,  0.35f, NULL, &dock_main_id);
            ImGuiID dock_id_bottom_left = ImGui::DockBuilderSplitNode(dock_id_left, ImGuiDir_Down,  0.57f, NULL, &dock_id_left);
            ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(      dock_main_id, ImGuiDir_Right, 0.50f, NULL, &dock_main_id);

            ImGui::DockBuilderDockWindow("###file"      , dock_id_left);
            ImGui::DockBuilderDockWindow("###palette"   , dock_id_bottom_left);
            ImGui::DockBuilderDockWindow("###preview00" , dock_main_id);
            ImGui::DockBuilderDockWindow("###render00"  , dock_id_right);
            ImGui::DockBuilderDockWindow("###edit00"    , dock_main_id);

            for (int i = 1; i <= 99; i++) {
                char buff[13];
                sprintf(buff, "###preview%02d", i);
                char buff2[12];
                sprintf(buff2, "###render%02d", i);
                char buff3[10];
                sprintf(buff3, "###edit%02d", i);

                ImGui::DockBuilderDockWindow(buff, dock_main_id);
                ImGui::DockBuilderDockWindow(buff2, dock_id_right);
                ImGui::DockBuilderDockWindow(buff3, dock_main_id);
            }
            ImGui::DockBuilderFinish(dockspace_id);
        }

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            ImGui::Begin("File Info###file");  // Create a window and append into it.
            if (ImGui::Button("Load Files...")) {
                Open_Files(&user_info, &counter, My_Variables.pxlFMT_FO_Pal);
            }

            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

            ImGui::End();

            Show_Palette_Window(&My_Variables, counter);

            for (int i = 0; i < counter; i++)
            {
                if (My_Variables.F_Prop[i].file_open_window)
                {
                    Show_Preview_Window(&My_Variables, i, &event);
                }
            }
        }

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        //glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
        //  For this specific demo app we could also call SDL_GL_MakeCurrent(window, gl_context) directly)
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
            SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
        }
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    write_cfg_file(&user_info);

    return 0;
}

void crash_detector() {
    printf("Crash detector: \n");
    if (_CrtDumpMemoryLeaks()) {
        fflush(stdout);
        fflush(stderr);
    }
}

void Show_Preview_Window(struct variables *My_Variables, int counter, SDL_Event* event)
{
    //shortcuts...possibly replace variables* with just LF*
    LF* F_Prop = &My_Variables->F_Prop[counter];
    SDL_PixelFormat* pxlFMT_FO_Pal = My_Variables->pxlFMT_FO_Pal;

    std::string a = F_Prop->c_name;
    char b[3];
    sprintf(b, "%02d", counter);
    std::string name = a + "###preview" + b;
    bool wrong_size;

    if (F_Prop->image == NULL) {
        wrong_size = NULL;
    }
    else {
        wrong_size = (F_Prop->image->w != 350) ||
                     (F_Prop->image->h != 300);
    }
    ImGui::Begin(name.c_str(), (&F_Prop->file_open_window), 0);
    //Editing buttons
    //TODO: for some reason map tile's won't edit, need to fix
    if (ImGui::Button("SDL Convert and Paint")) {
        Prep_Image(F_Prop,
                   pxlFMT_FO_Pal,
                   true,
                  &F_Prop->edit_image_window);
    }
    if (ImGui::Button("Euclidian Convert and Paint")) {
        Prep_Image(F_Prop,
                   pxlFMT_FO_Pal,
                   false,
                  &F_Prop->edit_image_window);
    }

    // Check image size to match tile size (350x300 pixels)
    if (wrong_size) {
        ImGui::Text("This image is the wrong size to make a tile...");
        ImGui::Text("Size is %dx%d",
                    F_Prop->image->w,
                    F_Prop->image->h);
        ImGui::Text("Tileable Map images need to be a multiple of 350x300 pixels");
        //Buttons
        if (ImGui::Button("Preview Tiles - SDL color match")) {
            Prep_Image(F_Prop,
                       pxlFMT_FO_Pal,
                       true,
                      &F_Prop->preview_tiles_window);
        }
        if (ImGui::Button("Preview Tiles - Euclidian color match")) {
            Prep_Image(F_Prop,
                       pxlFMT_FO_Pal,
                       false,
                      &F_Prop->preview_tiles_window);
        }
        if (ImGui::Button("Preview as Image - SDL color match")) {
            Prep_Image(F_Prop,
                       pxlFMT_FO_Pal,
                       true,
                      &F_Prop->preview_image_window);
        }
        if (ImGui::Button("Preview as Image - Euclidian color match")) {
            Prep_Image(F_Prop,
                       pxlFMT_FO_Pal,
                       false,
                      &F_Prop->preview_image_window);
        }
    }

    ImGui::Text(F_Prop->c_name);
    if (My_Variables->Palette_Update) {
        if(F_Prop->image == NULL) {}
        else {
            Update_Palette2(F_Prop->image,
                           &F_Prop->Optimized_Texture,
                            pxlFMT_FO_Pal);
        }
    }

    ImGui::Image(
        (ImTextureID)F_Prop->Optimized_Texture,
        ImVec2((float)F_Prop->image->w,
        (float)F_Prop->image->h),
        My_Variables->uv_min,
        My_Variables->uv_max,
        My_Variables->tint_col,
        My_Variables->border_col);

    // Draw red boxes to indicate where the tiles will be cut from
    if (wrong_size) {
        ImDrawList *Draw_List = ImGui::GetWindowDrawList();
        ImVec2 Origin = ImGui::GetItemRectMin();
        ImVec2 Top_Left = Origin;
        ImVec2 Bottom_Right = { 0, 0 };
        int max_box_x = F_Prop->image->w / 350;
        int max_box_y = F_Prop->image->h / 300;

        for (int i = 0; i < max_box_x; i++)
        {
            for (int j = 0; j < max_box_y; j++)
            {
                Top_Left.x = Origin.x + (i * 350);
                Top_Left.y = Origin.y + (j * 300);
                Bottom_Right = { Top_Left.x + 350, Top_Left.y + 300 };
                Draw_List->AddRect(Top_Left, Bottom_Right, 0xff0000ff, 0, 0, 5.0f);
            }
        }
        // Preview tiles from red boxes
        if (F_Prop->preview_tiles_window) {
            Preview_Tiles_Window(My_Variables, &Top_Left, &Bottom_Right,
                &Origin, &max_box_x, &max_box_y, counter);
        }
    }
    // Preview full image
    if (F_Prop->preview_image_window) {
        Show_Image_Render(My_Variables, &user_info, counter);
    }
    // Edit full image
    if (F_Prop->edit_image_window) {
        Edit_Image_Window(My_Variables, &user_info, counter, event);
    }
    ImGui::End();
}

void Show_Palette_Window(variables *My_Variables, int counter) {

    bool palette_window = true;
    std::string name = "Default Fallout palette ###palette";
    ImGui::Begin(name.c_str(), &palette_window);


    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {

            int index = y * 16 + x;
            SDL_Color color = My_Variables->pxlFMT_FO_Pal->palette->colors[index];
            float r = (float)color.r / 255.0f;
            float g = (float)color.g / 255.0f;
            float b = (float)color.b / 255.0f;

            char q[12];
            snprintf(q, 12, "%d##aa%d", index, index);
            if (ImGui::ColorButton(q, ImVec4(r, g, b, 1.0f))) {
                My_Variables->Color_Pick = (uint8_t)(index);
            }

            if (x < 15) ImGui::SameLine();

            if ((index) >= 229) {
                Cycle_Palette(My_Variables->pxlFMT_FO_Pal->palette,
                    &My_Variables->Palette_Update,
                    My_Variables->CurrentTime);
            }
        }
    }
    if (My_Variables->Palette_Update) {
        SDL_SetPaletteColors(My_Variables->pxlFMT_FO_Pal->palette,
            &My_Variables->pxlFMT_FO_Pal->palette->colors[228], 228, 28);
    }

    ImGui::End();
}

void Preview_Tiles_Window(variables *My_Variables,
    ImVec2 *Top_Left, ImVec2 *Bottom_Right, ImVec2 *Origin,
    int *max_box_x, int *max_box_y, int counter)
{
    std::string a = My_Variables->F_Prop[counter].c_name;
    char b[3];
    sprintf(b, "%02d", counter);
    std::string name = a + " Preview...###render" + b;

    ImGui::Begin(name.c_str(), &My_Variables->F_Prop[counter].preview_tiles_window, 0);

    if (ImGui::Button("Save as Map Tiles...")) {
        My_Variables->Render_Window = true;
        if (strcmp(My_Variables->F_Prop[counter].extension, "FRM") == 0)
        {
            Save_IMG(My_Variables->F_Prop[counter].image, &user_info);
        }
        else
        {
            Save_FRM_tiles(My_Variables->F_Prop[counter].Pal_Surface, &user_info);
        }
    }

    // Preview window for tiles already converted to palettized and dithered format
    if (My_Variables->F_Prop[counter].preview_tiles_window) {
        Top_Left = Origin;
        for (int y = 0; y < *max_box_y; y++)
        {
            for (int x = 0; x < *max_box_x; x++)
            {
                Top_Left->x = ((x * 350.0f)) / My_Variables->F_Prop[counter].image->w;
                Top_Left->y = ((y * 300.0f)) / My_Variables->F_Prop[counter].image->h;

                *Bottom_Right = { (Top_Left->x + (350.0f / My_Variables->F_Prop[counter].image->w)),
                                  (Top_Left->y + (300.0f / My_Variables->F_Prop[counter].image->h)) };

                ImGui::Image((ImTextureID)
                    My_Variables->F_Prop[counter].Optimized_Render_Texture,
                    ImVec2(350, 300),
                    *Top_Left,
                    *Bottom_Right,
                    My_Variables->tint_col,
                    My_Variables->border_col);

                ImGui::SameLine();
            }
            ImGui::NewLine();
        }
    }
    ImGui::End();
}

void Show_Image_Render(variables *My_Variables, struct user_info* user_info, int counter)
{
    char b[3];
    sprintf(b, "%02d", counter);
    std::string a = My_Variables->F_Prop[counter].c_name;
    std::string name = a + " Preview Window...###render" + b;

    ImGui::Begin(name.c_str(), &My_Variables->F_Prop[counter].preview_image_window, 0);
    if (ImGui::Button("Save as Image...")) {

        My_Variables->F_Prop[counter].preview_image_window = true;

        if (strcmp(My_Variables->F_Prop[counter].extension, "FRM") == 0)
        {
            Save_IMG(My_Variables->F_Prop[counter].image, user_info);
        }
        else
        {
            Save_FRM(My_Variables->F_Prop[counter].Pal_Surface, user_info);
        }
    }
    //ImVec2 Origin = ImGui::GetItemRectMin();
    //ImVec2 Top_Left = Origin;
    //ImVec2 Bottom_Right = { 0, 0 };
    //Bottom_Right = ImVec2(Top_Left.x + (My_Variables->F_Prop[counter].image->w),
    //                      Top_Left.y + (My_Variables->F_Prop[counter].image->h) );

    ImGui::Image((ImTextureID)
        My_Variables->F_Prop[counter].Optimized_Render_Texture,
        ImVec2(My_Variables->F_Prop[counter].image->w,
            My_Variables->F_Prop[counter].image->h));
    ImGui::End();
}

void Edit_Image_Window(variables *My_Variables, struct user_info* user_info, int counter, SDL_Event* event)
{
    char b[3];
    sprintf(b, "%02d", counter);
    std::string a = My_Variables->F_Prop[counter].c_name;
    std::string name = a + " Edit Window...###edit" + b;

    //TODO: crash detector?
    //crash_detector();

    if (!ImGui::Begin(name.c_str(), &My_Variables->F_Prop[counter].edit_image_window, 0))
    {
        ImGui::End();
        return;
    }
    else
    {
        if (!My_Variables->F_Prop[counter].edit_map_mask) {

            if (ImGui::Button("Save as Map Tiles...")) {
                Save_FRM_tiles(My_Variables->F_Prop[counter].Pal_Surface, user_info);
            }

            if (ImGui::Button("Create Mask Tiles...")) {
                My_Variables->F_Prop[counter].Map_Mask =
                      Create_Map_Mask(My_Variables->F_Prop[counter].image,
                                     &My_Variables->F_Prop[counter].Optimized_Mask_Texture,
                                     &My_Variables->F_Prop[counter].edit_image_window);
                My_Variables->F_Prop[counter].edit_map_mask = true;
            }

            ImGui::Image(
                (ImTextureID)My_Variables->F_Prop[counter].Optimized_Render_Texture,
                ImVec2(My_Variables->F_Prop[counter].image->w,
                       My_Variables->F_Prop[counter].image->h));

            Edit_Image(&My_Variables->F_Prop[counter], My_Variables->Palette_Update, event, &My_Variables->Color_Pick);

        }
        else {
            if (ImGui::Button("Export Mask Tiles...")) {
                //TODO: export mask tiles using msk2bmp2020 code
                Save_Map_Mask(My_Variables->F_Prop[counter].Map_Mask, user_info);
            }
            if (ImGui::Button("Load Mask Tiles...")) {
                //TODO: load mask tiles
                Load_Edit_MSK(&My_Variables->F_Prop[counter], user_info);
                //Load_Files(&My_Variables->F_Prop[counter],
                //            user_info,
                //            My_Variables->pxlFMT_FO_Pal);
            }

            ImGui::Image(
                (ImTextureID)My_Variables->F_Prop[counter].Optimized_Render_Texture,
                ImVec2(My_Variables->F_Prop[counter].image->w,
                       My_Variables->F_Prop[counter].image->h) );

            ImDrawList *Draw_List = ImGui::GetWindowDrawList();
            ImVec2 Origin = ImGui::GetItemRectMin();

            int width  = My_Variables->F_Prop[counter].image->w;
            int height = My_Variables->F_Prop[counter].image->h;
            ImVec2 Bottom_Right = { width + Origin.x, height + Origin.y };

            Edit_Map_Mask(&My_Variables->F_Prop[counter], event, &My_Variables->Palette_Update, Origin);

            //no alpha value input
            //Draw_List->AddImage((ImTextureID)My_Variables->F_Prop[counter].Optimized_Mask_Texture,
            //    Origin, Bottom_Right);
            //
            ////modifies alpha value input
            //int alpha = 64;
            //Draw_List->AddImage((ImTextureID)My_Variables->F_Prop[counter].Optimized_Mask_Texture,
            //              Origin, Bottom_Right, { 0, 0 }, { 1, 1 }, IM_COL32(255, 255, 255, alpha));

            if (ImGui::Button("Cancel Map Mask...")) {
                My_Variables->F_Prop[counter].edit_map_mask = false;
            }
        }

        //Removes the mask tile overlay
        if (ImGui::Button("Cancel Editing...")) {
            My_Variables->F_Prop[counter].edit_image_window = false;
        }

        ImGui::End();
    }
}


static void ShowMainMenuBar(int* counter)
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            ImGui::MenuItem("(demo menu)", NULL, false, false);
            if (ImGui::MenuItem("New")) {/*TODO: add a new file option w/blank surfaces*/ }
            if (ImGui::MenuItem("Open", "Ctrl+O"))
            { 
                Open_Files(&user_info, counter, My_Variables.pxlFMT_FO_Pal);
            }
            if (ImGui::MenuItem("Default Fallout Path"))
            {
                Set_Default_Path(&user_info);
            }
            //if (ImGui::BeginMenu("Open Recent")) {}
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit - currently unimplemented, work in progress"))
        {   //TODO: implement undo tree
            if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
            if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "CTRL+X")) {}
            if (ImGui::MenuItem("Copy", "CTRL+C")) {}
            if (ImGui::MenuItem("Paste", "CTRL+V")) {}
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

//TODO: Need to test wide character support
void Open_Files(struct user_info* user_info, int* counter, SDL_PixelFormat* pxlFMT) {
    // Assigns image to Load_Files.image and loads palette for the image
    // TODO: image needs to be less than 1 million pixels (1000x1000)
    // to be viewable in Titanium FRM viewer, what's the limit in the game?
    if (My_Variables.pxlFMT_FO_Pal == NULL)
    {
        printf("Error: Palette not loaded...");
        My_Variables.pxlFMT_FO_Pal = loadPalette("file name for palette here");
    }
    Load_Files(&My_Variables.F_Prop[*counter], user_info, My_Variables.pxlFMT_FO_Pal);

    Image2Texture(My_Variables.F_Prop[*counter].image,
                 &My_Variables.F_Prop[*counter].Optimized_Texture,
                 &My_Variables.F_Prop[*counter].file_open_window);

    if (My_Variables.F_Prop[*counter].c_name) { (*counter)++; }
}