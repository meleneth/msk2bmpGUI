#include <stdio.h>
#include <string.h>
// #include <stringapiset.h>
// #include <SDL_image.h>
#include <stb_image.h>

#ifdef QFO2_WINDOWS
#include <Windows.h>
#elif defined(QFO2_LINUX)
#include <unistd.h>
#endif

#include <filesystem>
#include <cstdint>
#include <system_error>
#include <algorithm>
#include <string_view>

#include "platform_io.h"

#include "Load_Files.h"
#include "Load_Animation.h"
#include "Load_Settings.h"
#include "tinyfiledialogs.h"
#include "Image2Texture.h"
#include "FRM_Convert.h"
#include "MSK_Convert.h"
#include "Edit_Image.h"

#include "display_FRM_OpenGL.h"

#include "timer_functions.h"
#include "ImGui_Warning.h"
#include <ImFileDialog.h>

char *Program_Directory()
{

#ifdef QFO2_WINDOWS
    wchar_t buff[MAX_PATH] = {};
    GetModuleFileNameW(NULL, buff, MAX_PATH);
    char *utf8_buff = strdup(tinyfd_utf16to8(buff));
#elif defined(QFO2_LINUX)
    char *utf8_buff = (char *)malloc(MAX_PATH * sizeof(char));

    ssize_t read_size = readlink("/proc/self/exe", utf8_buff, MAX_PATH - 1);
    if (read_size >= MAX_PATH || read_size == -1)
    {
        //TODO: log to file
        set_popup_warning(
            "[ERROR] Program Directory()\n\n"
            "Error reading program .exe location."
        );
        printf("Error reading program .exe location, read_size: %d", read_size);
        return NULL;
    }
    utf8_buff[read_size + 1] = '\0'; // append null to entire string

#endif

    char *ptr = strrchr(utf8_buff, PLATFORM_SLASH) + 1; // replace filename w/null leaving only directory
    *ptr = '\0';

    // MessageBoxW(NULL,
    //     buff,
    //     L"string",
    //     MB_ABORTRETRYIGNORE);

    return utf8_buff;
}

void handle_file_drop(char *file_name, LF *F_Prop, int *counter, shader_info *shaders)
{
    F_Prop->file_open_window = Drag_Drop_Load_Files(file_name, F_Prop, &F_Prop->img_data, shaders);

    if (F_Prop->c_name)
    {
        (*counter)++;
    }
}


bool open_multiple_files(std::vector<std::filesystem::path> path_vec,
                         LF *F_Prop, shader_info *shaders, bool *multiple_files,
                         int *counter, int *window_number_focus)
{
    char question[MAX_PATH + 50] = {};

//TODO: get rid of this garbage if u8string() works for windows
// #ifdef QFO2_WINDOWS
//     snprintf(buffer, MAX_PATH + 50, "%s\nIs this a group of sequential animation frames?",
//              tinyfd_utf16to8((*path_vec.begin()).parent_path().c_str()));
// #elif defined(QFO2_LINUX)
    snprintf(question, MAX_PATH + 50, "%s\nIs this a group of sequential animation frames?",
             (*path_vec.begin()).parent_path().u8string().c_str());
// #endif
    int type;
    //TODO: replace tinyfd_ stuff with ImFileDialog
    if (!(*multiple_files)) {   //false=ask question, true=automatic
        // returns 1 for yes, 2 for no, 0 for cancel
        type = tinyfd_messageBox("Animation? or Single Images?",
                                 question, "yesnocancel", "question", 2);
    }   //TODO: wtf is going on here? where's cancel button?
    else {
        type = 1;               //yes, animation, open automatically?
    }

    if (type == 2) {            //no, open each image individually
        for (const std::filesystem::path &path : path_vec) {
            F_Prop[*counter].file_open_window =
                Drag_Drop_Load_Files(path.u8string().c_str(),
                                     &F_Prop[*counter],
                                     &F_Prop[*counter].img_data,
                                     shaders);
            (*counter)++;
        }
        return true;
    }
    else if (type == 1) {       //yes it is "a group of sequential animation frames"
        *multiple_files = true;
        bool does_window_exist = (*window_number_focus > -1);
        int num = does_window_exist ? *window_number_focus : *counter;

        F_Prop[num].file_open_window = Drag_Drop_Load_Animation(path_vec, &F_Prop[num]);

        if (!does_window_exist) {
            (*window_number_focus) = (*counter);
            (*counter)++;
        }
        return true;
    }
    else if (type == 0) {
        return false;
    }
    return false;   //shouldn't have to do this, shouldn't be able to reach this line
}

#ifdef QFO2_WINDOWS
// Checks the file extension against known working extensions
// Returns true if any extension matches, else return false
bool Supported_Format(const std::filesystem::path &file)
{
    // array of compatible filetype extensions
    constexpr const static wchar_t supported[13][6]{
        L".FRM", L".MSK", L".PNG",
        L".JPG", L".JPEG", L".BMP",
        L".GIF",
        L".FR0", L".FR1", L".FR2", L".FR3", L".FR4", L".FR5"
        };
    int k = sizeof(supported) / (6 * sizeof(wchar_t));


    // actual extension check
    int i = 0;
    while (i < k)
    {
        //compare extension to determine if file is viewable
        if (io_wstrncmp(file.extension().c_str(), supported[i], 5) == 0)
        {
            return true;
        }
        i++;
    }

    return false;
}
#elif defined(QFO2_LINUX)
// Checks the file extension against known working extensions
// Returns true if any extension matches, else return false
bool Supported_Format(const std::filesystem::path &file)
{
    // array of compatible filetype extensions
    constexpr const static char supported[13][6]{
        ".FRM", ".MSK", ".PNG",
        ".BMP", ".JPG", ".JPEG",
        ".GIF",
        ".FR0",".FR1", ".FR2", ".FR3", ".FR4", ".FR5"
        };
    int k = sizeof(supported) / (6 * sizeof(char));

    // actual extension check
    int i = 0;
    while (i < k) {
        //compare extension to determine if file is viewable
        if (io_strncmp(file.extension().c_str(), supported[i], 5) == 0)
        {
            return true;
        }
        i++;
    }

    return false;
}
#endif

//TODO: delete, not used anywhere
// tried to handle a subdirectory in regular C, but didn't actually finish making this
char **handle_subdirectory_char(const std::filesystem::path &directory)
{
    char **array_of_paths = NULL;
    char *single_path = NULL;
    std::error_code error;
    int i = 0;

    for (const std::filesystem::directory_entry &file : std::filesystem::directory_iterator(directory))
    {
        bool is_subdirectory = file.is_directory(error);
        if (error)
        {
            printf("generic error message");
            return array_of_paths;
        }
        if (is_subdirectory)
        {
            // do nothing for now
            continue;
        }
        else
        {
            int str_length = strlen((char *)file.path().c_str());
            single_path = (char *)malloc(sizeof(char) * str_length);
            snprintf(single_path, str_length, "%s", file.path().c_str());

            array_of_paths[i] = single_path;
            i++;
        }
    }
    return array_of_paths;
}

std::vector<std::filesystem::path> handle_subdirectory_vec(const std::filesystem::path &directory)
{
    std::vector<std::filesystem::path> animation_images;
    std::error_code error;
    for (const std::filesystem::directory_entry &file : std::filesystem::directory_iterator(directory))
    {
        bool is_subdirectory = file.is_directory(error);
        if (error) {
            printf("error when checking if file_name is directory when loading file: %d", __LINE__);
            set_popup_warning(
                "[ERROR] When checking if file_name is directory when loading file"
            );
            return animation_images;
        }
        if (is_subdirectory) {
            // TODO: handle different directions in subdirectories?
            // animation_images = handle_subdirectory_vec(file.path());
            continue;
        }
        else if (Supported_Format(file)) {
            animation_images.push_back(file);
        }
    }

    uint64_t start_time = start_timer();

    // std::sort(animation_images.begin(), animation_images.end());                                        // ~50ms
    // std::sort(std::execution::seq, animation_images.begin(), animation_images.end());                   // ~50ms
    // std::sort(std::execution::seq, animation_images.begin(), animation_images.end(),
    //          [](std::filesystem::path& a, std::filesystem::path& b)
    //           { return (a.filename() < b.filename()); });                                               // ~35ms
    // std::sort(std::execution::seq, animation_images.begin(), animation_images.end(),
    //          [](std::filesystem::path& a, std::filesystem::path& b)
    //           { return (strcmp(a.filename().string().c_str(), b.filename().string().c_str())); });      // ~75ms
    // std::sort(std::execution::seq, animation_images.begin(), animation_images.end(),
    //          [](std::filesystem::path& a, std::filesystem::path& b)
    //           { return (wcscmp(a.c_str(), b.c_str()) < 0); });                                          // ~10ms
    // std::sort(std::execution::seq, animation_images.begin(), animation_images.end(),
    //         [](std::filesystem::path& a, std::filesystem::path& b)
    //          { return (a.native() < b.native()); });                                                    // ~17ms
    // std::sort(std::execution::seq, animation_images.begin(), animation_images.end(),
    //     [](std::filesystem::path& a, std::filesystem::path& b)
    //{ return (a < b); });                                                                               // ~47ms
    // std::sort(std::execution::seq, animation_images.begin(), animation_images.end(),
    //          [](std::filesystem::path& a, std::filesystem::path& b)
    //           { const wchar_t* a_file = wcspbrk(a.c_str(), L"/\\");
    //             const wchar_t* b_file = wcspbrk(b.c_str(), L"/\\");
    //             return (wcscmp(a_file, b_file) < 0); });                                                // ~13ms
    //  size_t parent_path_size = directory.native().size();
    //  std::sort(std::execution::seq, animation_images.begin(), animation_images.end(),
    //             [&parent_path_size](std::filesystem::path& a, std::filesystem::path& b)
    //              {
    //                const wchar_t* a_file = a.c_str() + parent_path_size;
    //                const wchar_t* b_file = b.c_str() + parent_path_size;
    //                return (wcscmp(a_file, b_file) < 0);
    //              });                                                                                     // ~1ms

    size_t parent_path_size = directory.native().size();
    std::sort(animation_images.begin(), animation_images.end(),
            [&parent_path_size](std::filesystem::path &a, std::filesystem::path &b)
            {
                int a_size = a.native().size();
                int b_size = b.native().size();
                int larger_size = (a_size < b_size) ? a_size : b_size;
#ifdef QFO2_WINDOWS
                return io_wstrncmp((a.c_str() + parent_path_size), (b.c_str() + parent_path_size), larger_size);
#elif defined(QFO2_LINUX)
                return io_strncmp((a.c_str() + parent_path_size), (b.c_str() + parent_path_size), larger_size);
#endif
            });

    // std::sort(std::execution::seq, animation_images.begin(), animation_images.end(),
    //            [](std::filesystem::path& a, std::filesystem::path& b)
    //             {
    //                 std::wstring_view a_v = a.native();
    //                 std::wstring_view b_v = b.native();
    //                 a_v = a_v.substr(a_v.find_last_of(L"\\/"));
    //                 b_v = b_v.substr(b_v.find_last_of(L"\\/"));
    //                 return (a_v < b_v);
    //             });                                                                                   // ~9ms
    // std::sort(std::execution::seq, animation_images.begin(), animation_images.end(),
    //            [](std::filesystem::path& a, std::filesystem::path& b)
    //             {
    //                 std::wstring_view a_v = a.native();
    //                 std::wstring_view b_v = b.native();
    //                 a_v = a_v.substr(a_v.find_last_of(L"\\/"));
    //                 b_v = b_v.substr(b_v.find_last_of(L"\\/"));
    //                 return (wcscmp(a_v.data(), b_v.data()) < 0);
    //             });                                                                                     // ~9ms
    // size_t parent_path_size = directory.native().size();
    // std::sort(std::execution::seq, animation_images.begin(), animation_images.end(),
    //             [&parent_path_size](std::filesystem::path& a, std::filesystem::path& b)
    //              {
    //                  std::wstring_view a_v = a.native();
    //                  std::wstring_view b_v = b.native();
    //                  a_v = a_v.substr(parent_path_size);
    //                  b_v = b_v.substr(parent_path_size);
    //                  return (a_v < b_v);
    //              });                                                                                    // ~2ms
    // size_t parent_path_size = directory.native().size();
    // std::sort(std::execution::seq, animation_images.begin(), animation_images.end(),
    //             [&parent_path_size](std::filesystem::path& a, std::filesystem::path& b)
    //              {
    //                  const std::wstring_view a_v = a.native().substr(parent_path_size);
    //                  const std::wstring_view b_v = b.native().substr(parent_path_size);
    //                  return (a_v < b_v);
    //              });                                                                                    // ~3ms

    print_timer(start_time);

    return animation_images;
}

#ifdef QFO2_WINDOWS
//Store directory files in memory for quick Next/Prev buttons
//Filepaths for files are assigned to pointers passed in
void Next_Prev_File(char *next, char *prev, char *frst, char *last, char *current)
{
    // LARGE_INTEGER StartingTime, EndingTime, ElapsedMicroseconds;
    // LARGE_INTEGER Frequency;
    // QueryPerformanceFrequency(&Frequency);
    // QueryPerformanceCounter(&StartingTime);

    std::filesystem::path file_path(current);
    const std::filesystem::path &directory = file_path.parent_path();
    size_t parent_path_size = directory.native().size();

    wchar_t *w_current = tinyfd_utf8to16(current);
    std::filesystem::path w_next;
    std::filesystem::path w_prev;
    std::filesystem::path w_frst;
    std::filesystem::path w_last;
    const wchar_t *iter_file;
    std::error_code error;
    for (const std::filesystem::directory_entry &file : std::filesystem::directory_iterator(directory))
    {
        bool is_subdirectory = file.is_directory(error);
        if (error)
        {
            // TODO: convert to tinyfd_filedialog() popup warning
            printf("error when checking if file_name is directory");
        }
        if (is_subdirectory)
        {
            // TODO: handle different directions in subdirectories?
            // handle_subdirectory(file.path());
            continue;
        }
        else
        {
            if (Supported_Format(file))
            {

                iter_file = (file.path().c_str() + parent_path_size);

                // if (w_frst.empty() || (wcscmp(iter_file, w_frst.c_str() + parent_path_size) < 0)) {
                if (w_frst.empty() || (CompareStringEx(LOCALE_NAME_USER_DEFAULT, LINGUISTIC_IGNORECASE,
                                                       iter_file, -1, (w_frst.c_str() + parent_path_size), -1,
                                                       NULL, NULL, NULL) -
                                           2 <
                                       0))
                {
                    w_frst = file;
                }
                // if (w_last.empty() || (wcscmp(iter_file, w_last.c_str() + parent_path_size) > 0)) {
                if (w_last.empty() || (CompareStringEx(LOCALE_NAME_USER_DEFAULT, LINGUISTIC_IGNORECASE,
                                                       iter_file, -1, (w_last.c_str() + parent_path_size), -1,
                                                       NULL, NULL, NULL) -
                                           2 >
                                       0))
                {
                    w_last = file;
                }

                // int cmp = wcscmp(iter_file, w_current + parent_path_size);
                int cmp = CompareStringEx(LOCALE_NAME_USER_DEFAULT, LINGUISTIC_IGNORECASE,
                                          iter_file, -1, (w_current + parent_path_size), -1,
                                          NULL, NULL, NULL) -
                          2;

                if (cmp < 0)
                {
                    // if (w_prev.empty() || (wcscmp(iter_file, w_prev.c_str() + parent_path_size) > 0)) {
                    if (w_prev.empty() || (CompareStringEx(LOCALE_NAME_USER_DEFAULT, LINGUISTIC_IGNORECASE,
                                                           iter_file, -1, (w_prev.c_str() + parent_path_size), -1,
                                                           NULL, NULL, NULL) -
                                               2 >
                                           0))
                    {
                        w_prev = file;
                    }
                }
                else if (cmp > 0)
                {
                    // if (w_next.empty() || (wcscmp(iter_file, w_next.c_str() + parent_path_size) < 0)) {
                    if (w_next.empty() || (CompareStringEx(LOCALE_NAME_USER_DEFAULT, LINGUISTIC_IGNORECASE,
                                                           iter_file, -1, (w_next.c_str() + parent_path_size), -1,
                                                           NULL, NULL, NULL) -
                                               2 <
                                           0))
                    {
                        w_next = file;
                    }
                }
            }
        }
    }

    if (w_prev.empty())
    {
        w_prev = w_last;
    }
    if (w_next.empty())
    {
        w_next = w_frst;
    }

    char *temp = tinyfd_utf16to8(w_prev.c_str());
    int temp_size = strlen(temp);
    memcpy(prev, temp, temp_size);
    prev[temp_size] = '\0';

    temp = tinyfd_utf16to8(w_next.c_str());
    temp_size = strlen(temp);
    memcpy(next, temp, temp_size);
    next[temp_size] = '\0';

    temp = tinyfd_utf16to8(w_frst.c_str());
    temp_size = strlen(temp);
    memcpy(frst, temp, temp_size);
    frst[temp_size] = '\0';

    temp = tinyfd_utf16to8(w_last.c_str());
    temp_size = strlen(temp);
    memcpy(last, temp, temp_size);
    last[temp_size] = '\0';

    // QueryPerformanceCounter(&EndingTime);
    // ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
    // ElapsedMicroseconds.QuadPart *= 1000000;
    // ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;

    // printf("Next_Prev_File time: %d\n", ElapsedMicroseconds.QuadPart);
}
#elif defined(QFO2_LINUX)

//Store directory files in memory for quick Next/Prev buttons
//Filepaths for files are assigned to the pointers passed in
void Next_Prev_File(char *next, char *prev, char *frst, char *last, char *current)
{
    // LARGE_INTEGER StartingTime, EndingTime, ElapsedMicroseconds;
    // LARGE_INTEGER Frequency;
    // QueryPerformanceFrequency(&Frequency);
    // QueryPerformanceCounter(&StartingTime);

    //TODO: I don't think this is hit anymore
    //      since I moved the check to Next_Prev_Buttons()
    if (!strlen(current)) {
        //TODO: make popup error
        return;
    }

    std::filesystem::path file_path(current);
    std::filesystem::path directory = file_path.parent_path();
    size_t parent_path_size = directory.native().size();

    std::filesystem::path l_next;
    std::filesystem::path l_prev;
    std::filesystem::path l_frst;
    std::filesystem::path l_last;
    const char *iter_file;
    std::error_code error;
    for (const std::filesystem::directory_entry &file : std::filesystem::directory_iterator(directory))
    {
        bool is_subdirectory = file.is_directory(error);
        if (error) {
            // TODO: convert to tinyfd_filedialog() popup warning
            printf("error when checking if file_name is directory");
        }
        if (is_subdirectory) {
            // TODO: handle different directions in subdirectories?
            // handle_subdirectory(file.path());
            continue;
        } else {
            if (Supported_Format(file)) {
                iter_file = (file.path().c_str() + parent_path_size);

                if (l_frst.empty() || strcasecmp(iter_file, (l_frst.c_str() + parent_path_size)) < 0)
                {
                    l_frst = file;
                }

                if (l_last.empty() || strcasecmp(iter_file, (l_last.c_str() + parent_path_size)) > 0)
                {
                    l_last = file;
                }

                int cmp = strcasecmp(iter_file, (current + parent_path_size));

                if (cmp < 0) {
                    if (l_prev.empty() || strcasecmp(iter_file, (l_prev.c_str() + parent_path_size)) > 0)
                    {
                        l_prev = file;
                    }
                } else if (cmp > 0) {
                    if (l_next.empty() || strcasecmp(iter_file, (l_next.c_str() + parent_path_size)) < 0)
                    {
                        l_next = file;
                    }
                }
            }
        }
    }

    if (l_prev.empty())
    {
        l_prev = l_last;
    }
    if (l_next.empty())
    {
        l_next = l_frst;
    }

    int temp_size = strlen(l_prev.c_str());
    memcpy(prev, l_prev.c_str(), temp_size);
    prev[temp_size] = '\0';

    temp_size = strlen(l_next.c_str());
    memcpy(next, l_next.c_str(), temp_size);
    next[temp_size] = '\0';

    temp_size = strlen(l_frst.c_str());
    memcpy(frst, l_frst.c_str(), temp_size);
    frst[temp_size] = '\0';

    temp_size = strlen(l_last.c_str());
    memcpy(last, l_last.c_str(), temp_size);
    last[temp_size] = '\0';

    // QueryPerformanceCounter(&EndingTime);
    // ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
    // ElapsedMicroseconds.QuadPart *= 1000000;
    // ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;

    // printf("Next_Prev_File time: %d\n", ElapsedMicroseconds.QuadPart);
}
#endif

// was testing out using a std::set instead of a std::vector, but because it was so slow
// ended up just storing the filename, making this kind of broken
std::set<std::filesystem::path> handle_subdirectory_set(const std::filesystem::path &directory)
{
    std::set<std::filesystem::path> animation_images;
    std::error_code error;
    for (const std::filesystem::directory_entry &file : std::filesystem::directory_iterator(directory))
    {
        bool is_subdirectory = file.is_directory(error);
        if (error)
        {
            // TODO: convert to tinyfd_filedialog() popup warning
            printf("error when checking if file_name is directory");
            return animation_images;
        }
        if (is_subdirectory)
        {
            // TODO: handle different directions in subdirectories
            // handle_subdirectory(file.path());
            continue;
        }
        else
        {
            // char buffer[MAX_PATH] = {};

            // char* temp_name = strrchr((char*)(file.path().u8string().c_str()), PLATFORM_SLASH);
            // snprintf(buffer, MAX_PATH, "%s", temp_name + 1);
            // std::filesystem::path temp_path(buffer);
            // animation_images.insert(temp_path);

            std::string u8file_path = file.path().u8string();
            char *temp_name = strrchr((char *)(u8file_path.c_str()), '/');
            if (!temp_name)
            {
                temp_name = strrchr((char *)(u8file_path.c_str()), '\\');
            }
            animation_images.insert(temp_name + 1);
        }
    }

    return animation_images;
}

// TODO: make a define switch for linux when I move to there
std::optional<bool> handle_directory_drop(char *file_name, LF *F_Prop, int *window_number_focus, int *counter,
                                          shader_info *shaders)
{
    char buffer[MAX_PATH];
#ifdef QFO2_WINDOWS
    //TODO: which method am I using?
    std::filesystem::path path(tinyfd_utf8to16(file_name));
    //std::filesystem::path path(file_name).u8string().c_str();

#elif defined(QFO2_LINUX)
    std::filesystem::path path(file_name);
#endif
    std::vector<std::filesystem::path> animation_images;

    std::error_code error;
    bool is_directory = std::filesystem::is_directory(path, error);
    if (error)
    {
        //TODO: log to file
        set_popup_warning(
            "[ERROR] handle_directory_drop()\n\n"
            "Error checking if dropped file is a directory.\n"
            "This usually happens when the text encoding is not UTF8/16."
        );
        // tinyfd_notifyPopup("Error checking if dropped file is a directory",
        //                    "This usually happens when the text encoding is not UTF8/16.",
        //                    "info");
        printf("error checking if file_name is directory");
        return std::nullopt;
    }

    if (!is_directory) {
        return false;
    }
    bool is_subdirectory = false;
    bool multiple_files  = false;
    for (const std::filesystem::directory_entry &file : std::filesystem::directory_iterator(path))
    {
        is_subdirectory = file.is_directory(error);
        if (error) {
            //TODO: log to file
            set_popup_warning(
                "[ERROR] handle_directory_drop()\n\n"
                "Error when checking if file_name is directory."
            );
            printf("error when checking if file_name is directory");
            return std::nullopt;
        }
        if (is_subdirectory) {
            // handle different directions (NE/SE/NW/SW...etc) in subdirectories (1 level so far)
            std::vector<std::filesystem::path> images = handle_subdirectory_vec(file.path());

            if (!images.empty()) {
                open_multiple_files(images, F_Prop, shaders, &multiple_files, counter, window_number_focus);
            }

            continue;
        }
        else {
            animation_images.push_back(file);
        }
    }
    if (is_subdirectory) {
        return true;
    }
    else {
        if (animation_images.empty()) {
            return false;
        }
        else {
            return open_multiple_files(animation_images, F_Prop, shaders, &multiple_files, counter, window_number_focus);
        }
    }

}

bool prep_extension(LF *F_Prop, user_info *usr_info, const char *file_name)
{
    snprintf(F_Prop->Opened_File, MAX_PATH, "%s", file_name);
    F_Prop->c_name = strrchr(F_Prop->Opened_File, PLATFORM_SLASH) + 1;
    if (!strrchr(F_Prop->Opened_File, '.')) {
        return false;
    }
    F_Prop->extension = strrchr(F_Prop->Opened_File, '.') + 1;

    // store filepaths in this directory for navigating through
    Next_Prev_File(F_Prop->Next_File,
                   F_Prop->Prev_File,
                   F_Prop->Frst_File,
                   F_Prop->Last_File,
                   F_Prop->Opened_File);

    if (usr_info != NULL)
    {
        std::filesystem::path file_path(F_Prop->Opened_File);
        snprintf(usr_info->default_load_path, MAX_PATH, "%s", file_path.parent_path().string().c_str());
    }
    // TODO: remove this printf 8==D
    printf("\nextension: %s\n", F_Prop->extension);
    return true;
}

bool Drag_Drop_Load_Files(const char *file_name, LF *F_Prop, image_data *img_data, shader_info *shaders)
{
    return File_Type_Check(F_Prop, shaders, img_data, file_name);
}


bool ImDialog_load_files(LF* F_Prop, image_data *img_data, struct user_info *usr_info, shader_info *shaders)
{
    //TODO: move this to some initializing function
    ifd::FileDialog::Instance().CreateTexture = [](uint8_t* data, int w, int h, char fmt) -> void* {
        GLuint tex;
        // https://github.com/dfranx/ImFileDialog
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, (fmt==0)?GL_BGRA:GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);

        return (void*)(uint64_t)tex;
    };
    ifd::FileDialog::Instance().DeleteTexture = [](void* tex) {
        GLuint texID = (uint64_t)tex;
        glDeleteTextures(1, &texID);
    };

    // char load_path[MAX_PATH];
    // snprintf(load_path, MAX_PATH, "%s/", usr_info->default_load_path);

    // char* load_name;
    static char load_name[MAX_PATH];
    if (ImGui::Button("Load File")) {
        const char* ext_filter;
            ext_filter = "FRM/MSK and image files"
            "(*.png;"
            // "*.apng;"
            "*.jpg;*.jpeg;*.frm;*.fr0-5;*.msk;)"
            "{.fr0,.FR0,.fr1,.FR1,.fr2,.FR2,.fr3,.FR3,.fr4,.FR4,.fr5,.FR5,"
                ".png,.jpg,.jpeg,"
                ".frm,.FRM,"
                ".msk,.MSK,"
            "}";

        char* folder = usr_info->default_load_path;
        ifd::FileDialog::Instance().Open("FileLoadDialog", "Load File", ext_filter, folder);
    }

    if (ifd::FileDialog::Instance().IsDone("FileLoadDialog")) {
        if (ifd::FileDialog::Instance().HasResult()) {
            std::string temp = ifd::FileDialog::Instance().GetResult().u8string();
            strncpy(load_name, temp.c_str(), temp.length()+1);
            strncpy(usr_info->default_load_path, temp.c_str(), temp.length()+1);
            char* ptr = strrchr(usr_info->default_load_path, PLATFORM_SLASH);
            *ptr = '\0';
        }
        ifd::FileDialog::Instance().Close();
    }
    if (strlen(load_name) < 1) {
        return false;
    }
    return File_Type_Check(F_Prop, shaders, img_data, load_name);
}

bool Load_Files(LF *F_Prop, image_data *img_data, struct user_info *usr_info, shader_info *shaders)
{
    char load_path[MAX_PATH];
    snprintf(load_path, MAX_PATH, "%s/", usr_info->default_load_path);
    const char *FilterPattern1[9] = {
        "*.bmp", "*.png", "*.frm",
        "*.msk", "*.jpg", "*.jpeg",
        "*.gif", "*.fr0", "-.fr5",
        };

    char *FileName = tinyfd_openFileDialog(
        "Open files...",
        load_path,
        5,
        FilterPattern1,
        NULL,
        1);

    if (!FileName) {
        return false;
    }
    return File_Type_Check(F_Prop, shaders, img_data, FileName);
}

//Check file extension to make sure it's one of the varieties of FRM
//TODO: maybe combine with Supported_Format()?
bool FRx_check(char *ext)
{
    if (
        (io_strncmp(ext, "FRM", 4) == 0)
     || (io_strncmp(ext, "FR0", 4) == 0)
     || (io_strncmp(ext, "FR1", 4) == 0)
     || (io_strncmp(ext, "FR2", 4) == 0)
     || (io_strncmp(ext, "FR3", 4) == 0)
     || (io_strncmp(ext, "FR4", 4) == 0)
     || (io_strncmp(ext, "FR5", 4) == 0))
    {
        return true;
    }
    return false;
}

//TODO: maybe combine with Supported_Format()?
bool File_Type_Check(LF *F_Prop, shader_info *shaders, image_data *img_data, const char *file_name)
{
    //TODO: make a function that checks if image has a different palette
    //      besides the default Fallout 1/2 palette
    //      also need to convert all float* palettes to Palette*
    bool success = prep_extension(F_Prop, NULL, file_name);
    if (!success) {
        return false;
    }
    // FRx_check checks extension to make sure it's one of the FRM variants (FRM, FR0, FR1...FR5)
    if (FRx_check(F_Prop->extension)) {
        // The new way to load FRM images using openGL
        F_Prop->file_open_window = load_FRM_OpenGL(F_Prop->Opened_File, img_data, shaders);
        img_data->type = FRM;

        // draw_FRM_to_framebuffer(shaders, img_data->width, img_data->height,
        //                         img_data->framebuffer, img_data->FRM_texture);
    }
    else if (io_strncmp(F_Prop->extension, "MSK", 4) == 0) {  // 0 == match
        F_Prop->file_open_window = Load_MSK_Tile_Surface(F_Prop->Opened_File, img_data);
        img_data->type = MSK;
        init_framebuffer(img_data);
        bool success = false;
        success = framebuffer_init(&img_data->render_texture, &F_Prop->img_data.framebuffer, 350, 300);
        if (!success) {
            //TODO: log to file
            set_popup_warning(
                "[ERROR] Load_MSK_File_SURFACE\n\n"
                "Image framebuffer failed to attach correctly?"
            );
            printf("Image framebuffer failed to attach correctly?\n");
            return false;
        }
        SURFACE_to_texture(
            img_data->MSK_srfc,
            img_data->MSK_texture,
            350, 300, 1);
        draw_texture_to_framebuffer(
            shaders->palette,
            shaders->render_FRM_shader,
            &shaders->giant_triangle,
            img_data->framebuffer,
            img_data->MSK_texture, 350, 300);
    }
    // do this for all other more common (generic) image types
    // TODO: add another type for known generic image types?
    else {
        Surface* temp_surface = nullptr;
        temp_surface = Load_File_to_RGBA(F_Prop->Opened_File);
        if (!temp_surface) {
            //TODO: log to file
            set_popup_warning(
                "[ERROR] File_Type_Check()\n\n"
                "Unable to load image."
            );
            printf("Unable to load image: %s\n", F_Prop->Opened_File);
            return false;
        }

        img_data->ANM_dir = (ANM_Dir*)malloc(sizeof(ANM_Dir) * 6);
        if (!img_data->ANM_dir) {
            //TODO: log to file
            set_popup_warning(
                "[ERROR] File_Type_Check()\n\n"
                "Unable to allocate memory for ANM_dir."
            );
            printf("Unable to allocate memory for ANM_dir: %d", __LINE__);
            return false;
        }
        //initialize allocated memory
        new (img_data->ANM_dir) ANM_Dir[6];

        //TODO: refactor this to correctly point to ANM_dir[dir]->frame_data[0?];
        img_data->ANM_dir[0].frame_data = (Surface**)malloc(sizeof(Surface*));
        if (!img_data->ANM_dir->frame_data) {
            //TODO: log to file
            set_popup_warning(
                "[ERROR] File_Type_Check()\n\n"
                "Unable to allocate memory for ANM_Frame."
            );
            printf("Unable to allocate memory for ANM_Frame: %d", __LINE__);
            free(img_data->ANM_dir);
            return false;
        } else {
            new (img_data->ANM_dir->frame_data) ANM_Frame;
        }

        //TODO: refactor this to correctly point to ANM_dir[dir]->frame_data[0].frame_start;
        img_data->ANM_dir[0].frame_data[0] = temp_surface;
        if (img_data->ANM_dir->frame_data) {
            img_data->width  = F_Prop->img_data.ANM_dir[0].frame_data[0]->w;
            img_data->height = F_Prop->img_data.ANM_dir[0].frame_data[0]->h;
            img_data->ANM_dir[0].num_frames = 1;

            img_data->type = OTHER;

            // TODO: rewrite this function
            F_Prop->file_open_window 
                = Image2Texture(img_data->ANM_dir[0].frame_data[0],
                                &img_data->FRM_texture);

            init_framebuffer(img_data);
            //assign display direction to same as image slot
            //so we can see the image on load
            img_data->display_orient_num = NE;
            img_data->display_frame_num  = 0;
        }
    }

    // if ((F_Prop->IMG_Surface == NULL) && F_Prop->img_data.type != FRM && F_Prop->img_data.type != MSK)
    if (img_data->ANM_dir != NULL)
    {
        if ((img_data->ANM_dir[img_data->display_orient_num].frame_data == NULL)
            && img_data->type != FRM
            && img_data->type != MSK)
        {
            //TODO: log to file
            set_popup_warning(
                "[ERROR] File_Type_Check()\n\n"
                "Unable to open image file."
            );
            printf("Unable to open image file %s!\n", F_Prop->Opened_File);
            return false;
        }
    }
    // Set display window to open
    return true;
}

//TODO: am I using load_tile_texture() anymore?
void load_tile_texture(GLuint *texture, char *file_name)
{
    Surface *surface = Load_File_to_RGBA(file_name);

    if (!glIsTexture(*texture))
    {
        glDeleteTextures(1, texture);
    }
    glGenTextures(1, texture);

    glBindTexture(GL_TEXTURE_2D, *texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // This is required on WebGL for non power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // Same

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pxls);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    FreeSurface(surface);

    printf("glError: %d\n", glGetError());
}
