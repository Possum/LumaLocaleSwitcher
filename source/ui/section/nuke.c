#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <3ds.h>
#include <3ds/services/fs.h>

#include "action/action.h"
#include "task/task.h"
#include "../../core/screen.h"
#include "../../core/util.h"
#include "../prompt.h"
#include "../error.h"
#include "section.h"

static void really_nuke(ui_view* view, void* data, bool response) {
    // prompt_destroy(view);

    char *path = (char*) data;

    // Add trailing slash if it don't exist (but it should)
    size_t path_len = strlen(path);
    if (path[path_len-1] != '/') {
        path[path_len] = '/';
        path[path_len+1] = '\0';
    }

    if(response) {
        // FS_Archive *sdmc_archive = util_get_sdmc_archive();
        FS_Archive sdmc_archive;

        if (R_SUCCEEDED(FSUSER_OpenArchive(&sdmc_archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY,"")))) {

            FS_Path* fs_path = util_make_path_utf8(path);

            Handle dir_handle;
            FSUSER_OpenDirectory(&dir_handle, sdmc_archive, *fs_path); // TODO error handling?

            u32 count = 0;
            FS_DirectoryEntry entry;

            u32 files_deleted = 0;

            while (R_SUCCEEDED(FSDIR_Read(dir_handle, &count, 1, &entry)) && count > 0) {
                char* filename_buf = (char*) calloc(PATH_MAX, sizeof(char));
                char* entry_name_utf8 = (char*) calloc(PATH_MAX, sizeof(char));
                utf16_to_utf8((uint8_t*) entry_name_utf8, entry.name, sizeof(entry.name) - 1);
                snprintf(filename_buf, PATH_MAX, "%s%s", path, entry_name_utf8);
                FS_Path* file_path = util_make_path_utf8(filename_buf);
                FSUSER_DeleteFile(sdmc_archive, *file_path); // TODO error handling

                util_free_path_utf8(file_path);

                free(filename_buf);
                free(entry_name_utf8);

                files_deleted++;
            } // while (R_SUCCEEDED(FSDIR_Read(dir_handle, &count, 1, &entry)) && count > 0)

            util_free_path_utf8(fs_path);

            char* msg = (char*) calloc(PATH_MAX+40, sizeof(char));

            FSUSER_CloseArchive(sdmc_archive);

            sprintf(msg, "Removed %ld files.", files_deleted);
            prompt_display("Success", msg, COLOR_TEXT, false, NULL, NULL, NULL, NULL);
        }
        else {
            error_display(false, data, NULL, "Failed to access filesystem");
        }
    }
}

void nuke() {
    char *path = (char*) calloc(PATH_MAX, sizeof(char));
    char *message = (char*) calloc(PATH_MAX+22, sizeof(char));
    char *pattern = "Are you sure?\nTHIS WILL DESTROY EVERYTHING IN:\n%s";

    util_get_locale_path(path, PATH_MAX);

    // Find the actual directory part
    // FIXME yeah this is gross...
    char* substr = strstr(path, "%s"); // Finds the first instance of "%s"
    path[strlen(path) - strlen(substr)] = '\0'; // Chops it off using "\0"

    sprintf(message, pattern, path);
    prompt_display("Confirmation", message, COLOR_IMPORTANT, true, path, NULL, NULL, really_nuke);
}
