#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <sys/syslimits.h>

#include <3ds.h>

#include "action/action.h"
#include "task/task.h"
#include "../../core/screen.h"
#include "../../core/util.h"
#include "section.h"

#define LOCALE_ITEMS_COUNT 6

typedef struct {
    list_item items[LOCALE_ITEMS_COUNT];
    u32 count;
    Handle cancelEvent;
    bool populated;
} locale_data;

typedef struct {
    config_info* info;
    bool* populated;
} config_action_data;

static list_item locale_items[LOCALE_ITEMS_COUNT] = {
#ifdef LUMA_NIGHTLY
        {"/luma/locales/titles/%s/locale.txt", COLOR_TEXT, action_change_locale_dir},
        {"/luma/locales/%s.txt", COLOR_OUTDATED, action_change_locale_dir},
#else
        {"/luma/locales/%s.txt", COLOR_TEXT, action_change_locale_dir},
        {"/luma/locales/titles/%s/locale.txt", COLOR_TEXT, action_change_locale_dir},
#endif
        {"/homebrew/3ds/SaltFW/locales/%s.txt", COLOR_TEXT, action_change_locale_dir},
        {"/aurei/locales/%s.txt", COLOR_OUTDATED, action_change_locale_dir},
        {"/SaltFW/locales/%s.txt", COLOR_OUTDATED, action_change_locale_dir},
        {"Other", COLOR_TEXT, action_pick_locale_dir},
};

static void config_draw_top(ui_view* view, void* data, float x1, float y1, float x2, float y2, list_item* selected) {
    // TODO anything useful to put here??
    /*
    if(selected != NULL && selected->data != NULL) {
        // TODO defined in ui/ui.c
        // ui_draw_title_info(view, selected->data, x1, y1, x2, y2);
    }
    */
}

static void config_update(ui_view* view, void* data, list_item** items, u32** itemCount, list_item* selected, bool selectedTouched) {
    locale_data* listData = (locale_data*) data;

    if(hidKeysDown() & KEY_B) {
        if(listData->cancelEvent != 0) {
            svcSignalEvent(listData->cancelEvent);
            while(svcWaitSynchronization(listData->cancelEvent, 0) == 0) {
                svcSleepThread(1000000);
            }

            listData->cancelEvent = 0;
        }

        ui_pop();
        free(listData);
        list_destroy(view);
        return;
    }

    if(!listData->populated) { // This probably should never trigger
        if(listData->cancelEvent != 0) {
            svcSignalEvent(listData->cancelEvent);
            while(svcWaitSynchronization(listData->cancelEvent, 0) == 0) {
                svcSleepThread(1000000);
            }

            listData->cancelEvent = 0;
        }

        for (int i = 0; i < LOCALE_ITEMS_COUNT; i++)
            listData->items[i] = locale_items[i];
        listData->populated = true;
        listData->count = LOCALE_ITEMS_COUNT;
    }

    if(selected != NULL && selected->data != NULL && (selectedTouched || (hidKeysDown() & KEY_A))) {
        void(*action)(config_info*, bool*) = (void(*)(config_info*, bool*)) selected->data;
        config_info* info = (config_info*) calloc(1, sizeof(config_info));
        strncpy(info->path, selected->name, PATH_MAX);

        list_destroy(view);
        ui_pop();

        action(info, false);

        free(info);

        return;
    }

    if(*itemCount != &listData->count || *items != listData->items) {
        *itemCount = &listData->count;
        *items = listData->items;
    }
}

void config_open() {
    char *path = (char*) calloc(PATH_MAX, sizeof(char));
    char *message = (char*) calloc(PATH_MAX+22, sizeof(char));
    if (util_get_locale_path(path, PATH_MAX)) {
        sprintf(message, "Locale Dir: %s", path);
    }
    else {
        sprintf(message, "Locale Dir: %s [default]", path);
    }
    locale_data* data = (locale_data*) calloc(1, sizeof(locale_data));

    // Populate the list
    for (int i = 0; i < LOCALE_ITEMS_COUNT; i++)
        data->items[i] = locale_items[i];
    data->count = LOCALE_ITEMS_COUNT;
    data->populated = true;

    list_display(message, "A: Select, B: Return", data, config_update, config_draw_top);
}
