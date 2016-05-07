#pragma once
#include <3ds/services/cfgu.h>

// These align with CFG_Region in 3ds/services/cfgu.h
typedef enum {
    JPN = 0,
    USA,
    EUR,
    AUS,
    CHN,
    KOR,
    TWN,
    RGN_MAX,
    RGN_NONE = -1
} Region;

// These align with CFG_Language in 3ds/services/cfgu.h
typedef enum {
    JP = 0,
    EN,
    FR,
    DE,
    IT,
    ES,
    ZH,
    KO,
    NL,
    PT,
    RU,
    TW,
    LNG_MAX,
    LNG_NONE = -1
} Language;

typedef struct {
    u64 title_id;
    char* title_id_str;
    Region region;
    Language language;
} Locale;

Region region_from_string(char* string);
Language language_from_string(char* string);
const char* region_to_string(Region region);
const char* language_to_string(Language language);

char* locale_path_for_title(u64 titleId);
Locale* locale_for_title(u64 titleId);
Region region_for_title(u64 titleId);
Language language_for_title(u64 titleId);

Result set_region_and_language_for_title(u64 titleId, Region region, Language language);
Result set_region_for_title(u64 titleId, Region region);
Result set_language_for_title(u64 titleId, Language language);
