#include <malloc.h>
#include <string.h>
#include <3ds.h>
#include "action.h"
#include "../../prompt.h"
#include "../../error.h"
#include "../../../core/screen.h"

static void action_write_locale_dir_config(ui_view* view, void* data, bool response) {
    config_info *info = (config_info*) data;

    if(response) {
        FILE* config_file = fopen("/locales.conf", "w"); // TODO hardcoding
        if (config_file != NULL) {
            fprintf(config_file, "%s\n", info->path);
            fclose(config_file);
            char* msg = (char*) calloc(PATH_MAX+18, sizeof(char));
            sprintf(msg, "Wrote %s to /locales.conf", info->path);
            prompt_display("Success", msg, COLOR_TEXT, false, NULL, NULL, NULL, NULL);
        }
        else {
            error_display(NULL, NULL, NULL, "Failed to write config.");
        }
    }
}

void action_change_locale_dir(config_info* info, bool* populated) {
    config_info* data = calloc(1, sizeof(config_info));
    strncpy(data->path, info->path, PATH_MAX);
    char *pattern = "Set locale path to %s";
    char *message = (char*) calloc(1, strlen(pattern) + strlen(info->path) + 1);
    sprintf(message, pattern, info->path);
    prompt_display("Confirmation", message, COLOR_TEXT, true, data, NULL, NULL, action_write_locale_dir_config);
}
