#include <3ds.h>
#include <stdlib.h>
#include <string.h>

#include "action.h"
#include "../task/task.h"
#include "../../error.h"
#include "../../prompt.h"
#include "../../../core/screen.h"
#include "../../../core/util.h"
#include "../../../locale.h"

static void action_remove_locale_file(ui_view* view, void* data, bool response) {
    if(response) {
        title_info* info = (title_info*) data;

        char* path = locale_path_for_title(info->titleId);

        FS_Archive sdmc_archive;

        if (R_SUCCEEDED(FSUSER_OpenArchive(&sdmc_archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY,"")))) {
            FS_Path* fs_path = util_make_path_utf8(path);

            FSUSER_DeleteFile(sdmc_archive, *fs_path); // TODO error handling

            util_free_path_utf8(fs_path);

            char* msg = (char*) calloc(PATH_MAX+10, sizeof(char));
            snprintf(msg, PATH_MAX+10, "%s\nRemoved!", path);
            prompt_display("Success", msg, COLOR_TEXT, false, NULL, NULL, NULL, NULL);
            FSUSER_CloseArchive(sdmc_archive);
        }
        else {
            prompt_display("Failed", "Couldn't access filesystem", COLOR_TEXT, false, NULL, NULL, NULL, NULL);
        }

        // Refresh locale info
        info->locale = locale_for_title(info->titleId);
    }
}

void action_use_system_default(title_info* info, bool* populated) {
    prompt_display("Confirmation", "Reset the locale data for this title?", COLOR_TEXT, true, info, NULL, ui_draw_title_info, action_remove_locale_file);
}
