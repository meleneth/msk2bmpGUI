add_library(QFO2ToolLib
  town_map_tiles.h
  town_map_tiles.cpp
  B_Endian.h
  B_Endian.cpp
  display_FRM_OpenGL.h
  display_FRM_OpenGL.cpp
  Edit_Animation.cpp
  Edit_Animation.h
  Edit_Image.cpp
  Edit_Image.h
  FRM_Convert.cpp
  FRM_Convert.h
  Image_Render.cpp
  Image_Render.h
  Image2Texture.cpp
  Image2Texture.h
  ImGui_Warning.cpp
  ImGui_Warning.h
  Load_Animation.cpp
  Load_Animation.h
  Load_Files.cpp
  Load_Files.h
  load_FRM_OpenGL.cpp
  load_FRM_OpenGL.h
  Load_Settings.cpp
  Load_Settings.h
  MiniSDL.h
  MiniSDL.cpp
  MSK_Convert.cpp
  MSK_Convert.h
  Palette_Cycle.cpp
  Palette_Cycle.h
  Preview_Image.cpp
  Preview_Image.h
  Preview_Tiles.cpp
  Preview_Tiles.h
  Save_Files.cpp
  Save_Files.h
  shader_class.cpp
  shader_class.h
  Zoom_Pan.cpp
  Zoom_Pan.h
  platform_io.h
  platform_io.cpp
  timer_functions.h
  timer_functions.cpp
  Edit_TILES_LST.h
  Edit_TILES_LST.cpp
  tiles_pattern.h
  tiles_pattern.cpp
  Proto_Files.h
  Proto_Files.cpp
)

#these need to be located after add_library()
#target (QFO2ToolLib) needs to be declared
#_BEFORE_ being added to compile defs
# Windows compile stuff
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
target_compile_definitions(
  QFO2ToolLib
  PUBLIC QFO2_WINDOWS
)

# Linux compile stuff
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
target_compile_definitions(
  QFO2ToolLib
  PUBLIC QFO2_LINUX
)
endif()

target_include_directories(
  QFO2ToolLib PUBLIC
  ../dependencies/GLAD/include
  ../dependencies/tinyfiledialogs
  ../dependencies/imgui-1.90.8-docking
  ../dependencies/GLFW/glfw-3.4/include/
  ../dependencies/stb
  ../dependencies/ImFileDialog
)
