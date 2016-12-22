#include <3ds.h>
#include <sys/syslimits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "locale.h"
#include "core/util.h"

Region region_from_string(char* string) {
    return
        strcmp(string, "JPN") == 0 ? JPN :
        strcmp(string, "USA") == 0 ? USA :
        strcmp(string, "EUR") == 0 ? EUR :
        strcmp(string, "AUS") == 0 ? AUS :
        strcmp(string, "CHN") == 0 ? CHN :
        strcmp(string, "KOR") == 0 ? KOR :
        strcmp(string, "TWN") == 0 ? TWN :
        RGN_NONE;
}

Language language_from_string(char* string) {
    return
        strcmp(string, "JP") == 0 ? JP :
        strcmp(string, "EN") == 0 ? EN :
        strcmp(string, "FR") == 0 ? FR :
        strcmp(string, "DE") == 0 ? DE :
        strcmp(string, "IT") == 0 ? IT :
        strcmp(string, "ES") == 0 ? ES :
        strcmp(string, "ZH") == 0 ? ZH :
        strcmp(string, "KO") == 0 ? KO :
        strcmp(string, "NL") == 0 ? NL :
        strcmp(string, "PT") == 0 ? PT :
        strcmp(string, "RU") == 0 ? RU :
        strcmp(string, "TW") == 0 ? TW :
        LNG_NONE;
}

static const char* _Region_Strings[] = { "JPN", "USA", "EUR", "AUS", "CHN", "KOR", "TWN" };

const char* region_to_string(Region region) {
    if (region == RGN_NONE || region >= RGN_MAX)
        return "System Default";
    return _Region_Strings[region];
}

static const char* _Language_Strings[] = { "JP", "EN", "FR", "DE", "IT", "ES", "ZH", "KO", "NL", "PT", "RU", "TW"};

const char* language_to_string(Language language) {
    if (language == LNG_NONE || language >= LNG_MAX)
        return "System Default";
    return _Language_Strings[language];
}

char* concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1)+strlen(s2)+1);
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

char* locale_path_for_title(u64 titleId) {
    char* path = calloc(PATH_MAX, sizeof(char));

    char title_id_str[17];
    snprintf(title_id_str, 17, "%016llX", titleId);
    title_id_str[16] = '\0';

    snprintf(path, PATH_MAX, "%s%s/locale.txt", "/luma/titles/", title_id_str);

    return path;
}

char* titleID_path_for_title(u64 titleId) {
    char* path = calloc(PATH_MAX, sizeof(char));

    char title_id_str[17];
    snprintf(title_id_str, 17, "%016llX", titleId);
    title_id_str[16] = '\0';

    snprintf(path, PATH_MAX, "%s%s/", "/luma/titles/", title_id_str);

    return path;
}

Locale* locale_for_title(u64 titleId) {

    FS_Archive sdmc_archive;

    Locale *locale_info = (Locale*) calloc(1, sizeof(Locale));

    // Defaults
    locale_info->region = RGN_NONE;
    locale_info->language = LNG_NONE;

    Result res;
    if (R_FAILED(res = FSUSER_OpenArchive(&sdmc_archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY,"")))) {
        return locale_info;
    }

    Handle handle;
    FS_Path* fs_path = util_make_path_utf8(locale_path_for_title(titleId));
    FSUSER_OpenFileDirectly(&handle, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY,""), *fs_path, FS_OPEN_READ, 0); // TODO error handling?

    char* buffer = (char*) calloc(7, sizeof(char)); // ex., "JPN JP\0"
    u32 bytes_read;
    FSFILE_Read(handle, &bytes_read, 0, buffer, 6);
    FSFILE_Close(handle);

    util_free_path_utf8(fs_path);

    FSUSER_CloseArchive(sdmc_archive);

    if (bytes_read < 6) { // we need at least "JPN JP"
        locale_info->region = RGN_NONE;
        locale_info->language = LNG_NONE;
        return locale_info;
    }

    buffer[bytes_read] = '\0';

    char* region_str = (char*) calloc(4, sizeof(char));
    char* lang_str = (char*) calloc(3, sizeof(char));
    if (sscanf(buffer, "%3s %2s", region_str, lang_str) < 2) {
        locale_info->region = RGN_NONE;
        locale_info->language = LNG_NONE;
    }
    else {
        locale_info->region = region_from_string(region_str);
        locale_info->language = language_from_string(lang_str);
    }

    return locale_info;
}

Region region_for_title(u64 titleId) {
    Locale* locale = locale_for_title(titleId);
    return locale->region;
}

Language language_for_title(u64 titleId) {
    Locale* locale = locale_for_title(titleId);
    return locale->language;
}

Result _set_locale_for_title(u64 titleId, Locale* locale) {
    char* titles_dir = (char*) calloc(PATH_MAX, sizeof(char));
    util_get_titles_path(titles_dir, PATH_MAX);
    FS_Archive sdmc_archive;

    Result res;
    if (R_FAILED(res = FSUSER_OpenArchive(&sdmc_archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY,"")))) return res;

    // Create the titles directory if it doesn't exist
    util_ensure_dir(&sdmc_archive, titles_dir);
    free(titles_dir);

    // Create the titleID directory if it doesn't exist
    util_ensure_dir(&sdmc_archive, titleID_path_for_title(titleId));
    free(titleID_path_for_title(titleId));
	
    FS_Path* fs_path = util_make_path_utf8(locale_path_for_title(titleId));
	
	// If this fails, probably means titleID directory does not exist
    Handle handle;
    if(R_SUCCEEDED(FSUSER_OpenFileDirectly(&handle, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY,""), *fs_path, FS_OPEN_WRITE | FS_OPEN_CREATE, 0))) {
        char* buffer = (char*) calloc(8, sizeof(char)); // ex: "JPN JP\0"
        snprintf(buffer, 7, "%s %s\n", region_to_string(locale->region),
                language_to_string(locale->language));
        buffer[7] = '\0';

        u32 bytes_written;
        FSFILE_Write(handle, &bytes_written, 0, buffer, 6, FS_WRITE_FLUSH);
        FSFILE_Close(handle);

        util_free_path_utf8(fs_path);
        FSUSER_CloseArchive(sdmc_archive);

        return bytes_written == 6 ? true : -1;
    }
    return -1;
}

Result set_region_and_language_for_title(u64 titleId, Region region, Language language) {
    Locale* locale = locale_for_title(titleId);

    Result result;

    // Default to system region/language
    if (region == RGN_NONE) {
        result = CFGU_SecureInfoGetRegion((u8*)&region);
        if (region <= RGN_NONE || region >= RGN_MAX)
            return result;
    }
    if (language == LNG_NONE) {
        result = CFGU_GetSystemLanguage((u8*)&language);
        if (language <= LNG_NONE || language >= LNG_MAX)
            return result;
    }

    locale->region = region;
    locale->language = language;
    return _set_locale_for_title(titleId, locale);
}

Result set_region_for_title(u64 titleId, Region region) {
    Locale* locale = locale_for_title(titleId);

    return set_region_and_language_for_title(titleId, region, locale->language);
}

Result set_language_for_title(u64 titleId, Language language) {
    Locale* locale = locale_for_title(titleId);

    return set_region_and_language_for_title(titleId, locale->region, language);
}
