#include <sys/syslimits.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <3ds.h>
#include <3ds/services/am.h>

#include "../../list.h"
#include "../../error.h"
#include "../../../core/util.h"
#include "../../../core/screen.h"
#include "../../../locale.h"
#include "task.h"

typedef struct {
    list_item* items;
    u32* count;
    u32 max;

    Handle cancelEvent;
} populate_titles_data;

typedef struct locale_entry_list {
    u64 titleId;
    bool has_entry;
    struct locale_entry_list *next;
} locale_entry_list;

/*
 * This is a cache of all the titles that have an entry in the locales directory
 * file at the time of lookup. Gets refreshed when the title list is refreshed.
 *
 * This cache prevents a lookup attempt on every title, but looking through it
 * will be slow as the number of entries increases
 */
static locale_entry_list *LocaleList = NULL;

locale_entry_list* find_entry_for_title_id(u64 titleId) {
    locale_entry_list *ll_next = LocaleList;
    do {
        if (ll_next == NULL) return NULL;
        if (ll_next->titleId == titleId) return ll_next;
    } while ((ll_next = ll_next->next));
    return NULL; // This should never trigger because of the do{} while
}

Result populate_locales() {
    locale_entry_list *ll_head = LocaleList;
    locale_entry_list *ll_next;

    // First free locale_list
    while (ll_head != NULL && ll_head->next != NULL) {
        ll_next = ll_head->next;
        free(ll_head);
        ll_head = ll_next;
    }
    LocaleList = NULL;
    ll_head = LocaleList;
    ll_next = ll_head;

    char titles_path[PATH_MAX];
	util_get_titles_path(titles_path, PATH_MAX);

    FS_Archive sdmc_archive;

    Result res;
    if (R_FAILED(res = FSUSER_OpenArchive(&sdmc_archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY,"")))) return res;
    FS_Path* fs_path = util_make_path_utf8(titles_path);

    Handle dir_handle;

    FSUSER_OpenDirectory(&dir_handle, sdmc_archive, *fs_path); // TODO error handling?

    util_free_path_utf8(fs_path);

    u32 count = 0;
    FS_DirectoryEntry entry;

    while (R_SUCCEEDED(FSDIR_Read(dir_handle, &count, 1, &entry)) && count > 0) {
        // Check if the first 16 characters looks like a titleId
        // This is a fuzzy check but gud nuff
        char titleId_buf[17] = "";
        for (int i = 0; i < 16; i++) {
            if (isxdigit(entry.name[i])) {
                titleId_buf[i] = entry.name[i];
            }
            else {
                strncpy(titleId_buf, "", 1);
                break;
            }
        }
        if (strlen(titleId_buf) > 0) {
            u64 titleId = (u64) strtoll(titleId_buf, NULL, 16);
            if (ll_next == NULL) {
                // Initialize the list
                ll_next = (locale_entry_list*) calloc(1, sizeof(locale_entry_list));
                LocaleList = ll_next;
            }
            else {
                ll_next->next = (locale_entry_list*) calloc(1, sizeof(locale_entry_list));
                ll_next = ll_next->next;
            }
            ll_next->titleId = titleId;
            ll_next->has_entry = true;
            ll_next->next = NULL;
        }
    } // while (R_SUCCEEDED(FSDIR_Read(dir_handle, &count, 1, &entry)) && count > 0)

    return FSUSER_CloseArchive(sdmc_archive);
}

static Result task_populate_titles_add_ctr(populate_titles_data* data, FS_MediaType mediaType, u64 titleId) {
    Result res = 0;

    AM_TitleEntry entry;
    if(R_SUCCEEDED(res = AM_GetTitleInfo(mediaType, 1, &titleId, &entry))) {
        title_info* titleInfo = (title_info*) calloc(1, sizeof(title_info));
        if(titleInfo != NULL) {
            titleInfo->mediaType = mediaType;
            titleInfo->titleId = titleId;
            AM_GetTitleProductCode(mediaType, titleId, titleInfo->productCode);
            titleInfo->version = entry.version;
            titleInfo->installedSize = entry.size;
            titleInfo->twl = false;
            titleInfo->hasMeta = false;

            locale_entry_list* locale_entry = find_entry_for_title_id(titleId);
            if (locale_entry != NULL && locale_entry->has_entry) {
                titleInfo->locale = locale_for_title(titleId);
            }
            else {
                titleInfo->locale = calloc(1, sizeof(Locale));
                titleInfo->locale->region = RGN_NONE;
                titleInfo->locale->language = LNG_NONE;
            }

            list_item* item = &data->items[*data->count];

            static const u32 filePathData[] = {0x00000000, 0x00000000, 0x00000002, 0x6E6F6369, 0x00000000};
            static const FS_Path filePath = (FS_Path) {PATH_BINARY, 0x14, (u8*) filePathData};
            u32 archivePath[] = {(u32) (titleId & 0xFFFFFFFF), (u32) ((titleId >> 32) & 0xFFFFFFFF), mediaType, 0x00000000};

            FS_Path fs_path = {PATH_BINARY, 0x10, (u8*) archivePath};
            FS_Archive archive = {ARCHIVE_SAVEDATA_AND_CONTENT};

            Handle fileHandle;
            if(R_SUCCEEDED(FSUSER_OpenFileDirectly(&fileHandle, archive, fs_path, filePath, FS_OPEN_READ, 0))) {
                SMDH* smdh = (SMDH*) calloc(1, sizeof(SMDH));
                if(smdh != NULL) {
                    u32 bytesRead;
                    if(R_SUCCEEDED(FSFILE_Read(fileHandle, &bytesRead, 0, smdh, sizeof(SMDH))) && bytesRead == sizeof(SMDH)) {
                        if(smdh->magic[0] == 'S' && smdh->magic[1] == 'M' && smdh->magic[2] == 'D' && smdh->magic[3] == 'H') {
                            titleInfo->hasMeta = true;

                            u8 systemLanguage = CFG_LANGUAGE_EN;
                            CFGU_GetSystemLanguage(&systemLanguage);

                            utf16_to_utf8((uint8_t*) item->name, smdh->titles[systemLanguage].shortDescription, NAME_MAX - 1);

                            utf16_to_utf8((uint8_t*) titleInfo->meta.shortDescription, smdh->titles[systemLanguage].shortDescription, sizeof(titleInfo->meta.shortDescription) - 1);
                            utf16_to_utf8((uint8_t*) titleInfo->meta.longDescription, smdh->titles[systemLanguage].longDescription, sizeof(titleInfo->meta.longDescription) - 1);
                            utf16_to_utf8((uint8_t*) titleInfo->meta.publisher, smdh->titles[systemLanguage].publisher, sizeof(titleInfo->meta.publisher) - 1);
                            titleInfo->meta.texture = screen_load_texture_tiled_auto(smdh->largeIcon, sizeof(smdh->largeIcon), 48, 48, GPU_RGB565, false);
                        }
                    }

                    free(smdh);
                }

                FSFILE_Close(fileHandle);
            }

            bool empty = strlen(item->name) == 0;
            if(!empty) {
                empty = true;

                char* curr = item->name;
                while(*curr) {
                    if(*curr != ' ') {
                        empty = false;
                        break;
                    }

                    curr++;
                }
            }

            if(empty) {
                snprintf(item->name, NAME_MAX, "%016llX", titleId);
            }

            if(mediaType == MEDIATYPE_NAND) {
                item->rgba = COLOR_NAND;
            } else if(mediaType == MEDIATYPE_SD) {
                item->rgba = COLOR_SD;
            } else if(mediaType == MEDIATYPE_GAME_CARD) {
                item->rgba = COLOR_GAME_CARD;
            }

            item->data = titleInfo;

            (*data->count)++;
        } else {
            res = R_FBI_OUT_OF_MEMORY;
        }
    }

    return res;
}

static Result task_populate_titles_add_twl(populate_titles_data* data, FS_MediaType mediaType, u64 titleId) {
    Result res = 0;

    u64 realTitleId = 0;
    char productCode[12] = {'\0'};
    u16 version = 0;
    u64 installedSize = 0;

    AM_TitleEntry entry;
    if(R_SUCCEEDED(res = AM_GetTitleInfo(mediaType, 1, &titleId, &entry))) {
        realTitleId = titleId;
        AM_GetTitleProductCode(mediaType, titleId, productCode);
        version = entry.version;
        installedSize = entry.size;
    } else {
        u8* header = (u8*) calloc(1, 0x3B4);
        if(header != NULL) {
            if(R_SUCCEEDED(res = FSUSER_GetLegacyRomHeader(mediaType, titleId, header))) {
                realTitleId = titleId != 0 ? titleId : *(u64*) &header[0x230];
                memcpy(productCode, header, 0x00C);
                version = header[0x01E];
                installedSize = (header[0x012] & 0x2) != 0 ? *(u32*) &header[0x210] : *(u32*) &header[0x080];
            }

            free(header);
        } else {
            res = R_FBI_OUT_OF_MEMORY;
        }
    }

    if(R_SUCCEEDED(res)) {
        title_info* titleInfo = (title_info*) calloc(1, sizeof(title_info));
        if(titleInfo != NULL) {
            titleInfo->mediaType = mediaType;
            titleInfo->titleId = realTitleId;
            strncpy(titleInfo->productCode, productCode, 12);
            titleInfo->version = version;
            titleInfo->installedSize = installedSize;
            titleInfo->twl = true;
            titleInfo->hasMeta = false;

            locale_entry_list* locale_entry = find_entry_for_title_id(titleId);
            if (locale_entry != NULL && locale_entry->has_entry) {
                titleInfo->locale = locale_for_title(titleId);
            }
            else {
                titleInfo->locale = calloc(1, sizeof(Locale));
                titleInfo->locale->region = RGN_NONE;
                titleInfo->locale->language = LNG_NONE;
            }

            list_item* item = &data->items[*data->count];

            BNR* bnr = (BNR*) calloc(1, sizeof(BNR));
            if(bnr != NULL) {
                if(R_SUCCEEDED(FSUSER_GetLegacyBannerData(mediaType, titleId, (u8*) bnr))) {
                    titleInfo->hasMeta = true;

                    u8 systemLanguage = CFG_LANGUAGE_EN;
                    CFGU_GetSystemLanguage(&systemLanguage);

                    char title[0x100] = {'\0'};
                    utf16_to_utf8((uint8_t*) title, bnr->titles[systemLanguage], sizeof(title) - 1);

                    if(strchr(title, '\n') == NULL) {
                        size_t len = strlen(title);
                        strncpy(item->name, title, len);
                        strncpy(titleInfo->meta.shortDescription, title, len);
                    } else {
                        char* destinations[] = {titleInfo->meta.shortDescription, titleInfo->meta.longDescription, titleInfo->meta.publisher};
                        int currDest = 0;

                        char* last = title;
                        char* curr = NULL;

                        while(currDest < 3 && (curr = strchr(last, '\n')) != NULL) {
                            strncpy(destinations[currDest++], last, curr - last);
                            last = curr + 1;
                            *curr = ' ';
                        }

                        strncpy(item->name, title, last - title);
                        if(currDest < 3) {
                            strncpy(destinations[currDest], last, strlen(title) - (last - title));
                        }
                    }

                    u8 icon[32 * 32 * 2];
                    for(u32 x = 0; x < 32; x++) {
                        for(u32 y = 0; y < 32; y++) {
                            u32 srcPos = (((y >> 3) * 4 + (x >> 3)) * 8 + (y & 7)) * 4 + ((x & 7) >> 1);
                            u32 srcShift = (x & 1) * 4;
                            u16 srcPx = bnr->mainIconPalette[(bnr->mainIconBitmap[srcPos] >> srcShift) & 0xF];

                            u8 r = (u8) (srcPx & 0x1F);
                            u8 g = (u8) ((srcPx >> 5) & 0x1F);
                            u8 b = (u8) ((srcPx >> 10) & 0x1F);

                            u16 reversedPx = (u16) ((r << 11) | (g << 6) | (b << 1) | 1);

                            u32 dstPos = (y * 32 + x) * 2;
                            icon[dstPos + 0] = (u8) (reversedPx & 0xFF);
                            icon[dstPos + 1] = (u8) ((reversedPx >> 8) & 0xFF);
                        }
                    }

                    titleInfo->meta.texture = screen_load_texture_auto(icon, sizeof(icon), 32, 32, GPU_RGBA5551, false);
                }

                free(bnr);
            }

            bool empty = strlen(item->name) == 0;
            if(!empty) {
                empty = true;

                char* curr = item->name;
                while(*curr) {
                    if(*curr != ' ') {
                        empty = false;
                        break;
                    }

                    curr++;
                }
            }

            if(empty) {
                snprintf(item->name, NAME_MAX, "%016llX", realTitleId);
            }

            item->rgba = COLOR_DS_TITLE;
            item->data = titleInfo;

            (*data->count)++;
        } else {
            res = R_FBI_OUT_OF_MEMORY;
        }
    }

    return res;
}

static Result task_populate_titles_from(populate_titles_data* data, FS_MediaType mediaType, bool useDSiWare) {
    bool inserted;
    FS_CardType type;
    if(mediaType == MEDIATYPE_GAME_CARD && (R_FAILED(FSUSER_CardSlotIsInserted(&inserted)) || !inserted || R_FAILED(FSUSER_GetCardType(&type)))) {
        return 0;
    }

    Result res = 0;

    if(mediaType != MEDIATYPE_GAME_CARD || type == CARD_CTR) {
        u32 titleCount = 0;
        if(R_SUCCEEDED(res = AM_GetTitleCount(mediaType, &titleCount))) {
            u64* titleIds = (u64*) calloc(titleCount, sizeof(u64));
            if(titleIds != NULL) {
                if(R_SUCCEEDED(res = AM_GetTitleList(&titleCount, mediaType, titleCount, titleIds))) {
                    qsort(titleIds, titleCount, sizeof(u64), util_compare_u64);

                    for(u32 i = 0; i < titleCount && *data->count < data->max && R_SUCCEEDED(res); i++) {
                        if(task_is_quit_all() || svcWaitSynchronization(data->cancelEvent, 0) == 0) {
                            break;
                        }

                        bool dsiWare = ((titleIds[i] >> 32) & 0x8000) != 0;
                        if(dsiWare != useDSiWare) {
                            continue;
                        }

                        res = dsiWare ? task_populate_titles_add_twl(data, mediaType, titleIds[i]) : task_populate_titles_add_ctr(data, mediaType, titleIds[i]);
                    }
                }

                free(titleIds);
            } else {
                res = R_FBI_OUT_OF_MEMORY;
            }
        }
    } else {
        res = task_populate_titles_add_twl(data, mediaType, 0);
    }

    return res;
}

static void task_populate_titles_thread(void* arg) {
    populate_titles_data* data = (populate_titles_data*) arg;

    Result res = 0;
    if(R_FAILED(res = task_populate_titles_from(data, MEDIATYPE_GAME_CARD, false)) || R_FAILED(res = task_populate_titles_from(data, MEDIATYPE_SD, false)) || R_FAILED(res = task_populate_titles_from(data, MEDIATYPE_NAND, false)) || R_FAILED(res = task_populate_titles_from(data, MEDIATYPE_NAND, true))) {
        error_display_res(NULL, NULL, NULL, res, "Failed to load title listing.");
    }

    svcCloseHandle(data->cancelEvent);
    free(data);
}

void task_clear_titles(list_item* items, u32* count) {
    if(items == NULL || count == NULL) {
        return;
    }

    u32 prevCount = *count;
    *count = 0;

    for(u32 i = 0; i < prevCount; i++) {
        if(items[i].data != NULL) {
            title_info* titleInfo = (title_info*) items[i].data;
            if(titleInfo->hasMeta) {
                screen_unload_texture(titleInfo->meta.texture);
            }

            free(items[i].data);
            items[i].data = NULL;
        }

        memset(items[i].name, '\0', NAME_MAX);
        items[i].rgba = 0;
    }
}

Handle task_populate_titles(list_item* items, u32* count, u32 max) {
    if(items == NULL || count == NULL || max == 0) {
        return 0;
    }

    task_clear_titles(items, count);
    populate_locales(); // Clear and reinitialize LocaleList

    populate_titles_data* data = (populate_titles_data*) calloc(1, sizeof(populate_titles_data));
    if(data == NULL) {
        error_display(NULL, NULL, NULL, "Failed to allocate title list data.");

        return 0;
    }

    data->items = items;
    data->count = count;
    data->max = max;

    Result eventRes = svcCreateEvent(&data->cancelEvent, 1);
    if(R_FAILED(eventRes)) {
        error_display_res(NULL, NULL, NULL, eventRes, "Failed to create title list cancel event.");

        free(data);
        return 0;
    }

    if(threadCreate(task_populate_titles_thread, data, 0x4000, 0x18, 1, true) == NULL) {
        error_display(NULL, NULL, NULL, "Failed to create title list thread.");

        svcCloseHandle(data->cancelEvent);
        free(data);
        return 0;
    }

    return data->cancelEvent;
}
