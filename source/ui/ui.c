#include <stdio.h>
#include <string.h>
#include <time.h>

#include <3ds.h>

#include "section/task/task.h"
#include "ui.h"
#include "../core/screen.h"
#include "../locale.h"

#define MAX_UI_VIEWS 16

static ui_view* ui_stack[MAX_UI_VIEWS];
static int ui_stack_top = -1;

static Handle ui_stack_mutex = 0;

void ui_init() {
    if(ui_stack_mutex == 0) {
        svcCreateMutex(&ui_stack_mutex, false);
    }
}

void ui_exit() {
    if(ui_stack_mutex != 0) {
        svcCloseHandle(ui_stack_mutex);
        ui_stack_mutex = 0;
    }
}

ui_view* ui_top() {
    svcWaitSynchronization(ui_stack_mutex, U64_MAX);

    ui_view* ui = NULL;
    if(ui_stack_top >= 0) {
        ui = ui_stack[ui_stack_top];
    }

    svcReleaseMutex(ui_stack_mutex);

    return ui;
}

bool ui_push(ui_view* view) {
    if(view == NULL) {
        return false;
    }

    svcWaitSynchronization(ui_stack_mutex, U64_MAX);

    bool space = ui_stack_top < MAX_UI_VIEWS - 1;
    if(space) {
        ui_stack[++ui_stack_top] = view;
    }

    svcReleaseMutex(ui_stack_mutex);

    return space;
}

void ui_pop() {
    svcWaitSynchronization(ui_stack_mutex, U64_MAX);

    if(ui_stack_top >= 0) {
        ui_stack[ui_stack_top--] = NULL;
    }

    svcReleaseMutex(ui_stack_mutex);
}

static void ui_draw_top(ui_view* ui) {
    u32 topScreenBgWidth = 0;
    u32 topScreenBgHeight = 0;
    screen_get_texture_size(&topScreenBgWidth, &topScreenBgHeight, TEXTURE_TOP_SCREEN_BG);

    u32 topScreenTopBarWidth = 0;
    u32 topScreenTopBarHeight = 0;
    screen_get_texture_size(&topScreenTopBarWidth, &topScreenTopBarHeight, TEXTURE_TOP_SCREEN_TOP_BAR);

    u32 topScreenTopBarShadowWidth = 0;
    u32 topScreenTopBarShadowHeight = 0;
    screen_get_texture_size(&topScreenTopBarShadowWidth, &topScreenTopBarShadowHeight, TEXTURE_TOP_SCREEN_TOP_BAR_SHADOW);

    u32 topScreenBottomBarWidth = 0;
    u32 topScreenBottomBarHeight = 0;
    screen_get_texture_size(&topScreenBottomBarWidth, &topScreenBottomBarHeight, TEXTURE_TOP_SCREEN_BOTTOM_BAR);

    u32 topScreenBottomBarShadowWidth = 0;
    u32 topScreenBottomBarShadowHeight = 0;
    screen_get_texture_size(&topScreenBottomBarShadowWidth, &topScreenBottomBarShadowHeight, TEXTURE_TOP_SCREEN_BOTTOM_BAR_SHADOW);

    screen_select(GFX_TOP);
    screen_draw_texture(TEXTURE_TOP_SCREEN_BG, (TOP_SCREEN_WIDTH - topScreenBgWidth) / 2, (TOP_SCREEN_HEIGHT - topScreenBgHeight) / 2, topScreenBgWidth, topScreenBgHeight);

    if(ui->drawTop != NULL) {
        ui->drawTop(ui, ui->data, 0, topScreenTopBarHeight, TOP_SCREEN_WIDTH, TOP_SCREEN_HEIGHT - topScreenBottomBarHeight);
    }

    float topScreenTopBarX = (TOP_SCREEN_WIDTH - topScreenTopBarWidth) / 2;
    float topScreenTopBarY = 0;
    screen_draw_texture(TEXTURE_TOP_SCREEN_TOP_BAR, topScreenTopBarX, topScreenTopBarY, topScreenTopBarWidth, topScreenTopBarHeight);
    screen_draw_texture(TEXTURE_TOP_SCREEN_TOP_BAR_SHADOW, topScreenTopBarX, topScreenTopBarY + topScreenTopBarHeight, topScreenTopBarShadowWidth, topScreenTopBarShadowHeight);

    float topScreenBottomBarX = (TOP_SCREEN_WIDTH - topScreenBottomBarWidth) / 2;
    float topScreenBottomBarY = TOP_SCREEN_HEIGHT - topScreenBottomBarHeight;
    screen_draw_texture(TEXTURE_TOP_SCREEN_BOTTOM_BAR, topScreenBottomBarX, topScreenBottomBarY, topScreenBottomBarWidth, topScreenBottomBarHeight);
    screen_draw_texture(TEXTURE_TOP_SCREEN_BOTTOM_BAR_SHADOW, topScreenBottomBarX, topScreenBottomBarY - topScreenBottomBarShadowHeight, topScreenBottomBarShadowWidth, topScreenBottomBarShadowHeight);

    time_t t = time(NULL);
    char* timeText = ctime(&t);

    float timeTextWidth;
    float timeTextHeight;
    screen_get_string_size(&timeTextWidth, &timeTextHeight, timeText, 0.5f, 0.5f);
    screen_draw_string(timeText, topScreenTopBarX + (topScreenTopBarWidth - timeTextWidth) / 2, topScreenTopBarY, 0.5f, 0.5f, COLOR_TEXT, false);

    u32 batteryIcon = 0;
    u8 batteryChargeState = 0;
    u8 batteryLevel = 0;
    if(R_SUCCEEDED(PTMU_GetBatteryChargeState(&batteryChargeState)) && batteryChargeState) {
        batteryIcon = TEXTURE_BATTERY_CHARGING;
    } else if(R_SUCCEEDED(PTMU_GetBatteryLevel(&batteryLevel))) {
        batteryIcon = TEXTURE_BATTERY_0 + batteryLevel;
    } else {
        batteryIcon = TEXTURE_BATTERY_0;
    }

    u32 batteryWidth;
    u32 batteryHeight;
    screen_get_texture_size(&batteryWidth, &batteryHeight, batteryIcon);

    float batteryX = topScreenTopBarX + topScreenTopBarWidth - 2 - batteryWidth;
    float batteryY = topScreenTopBarY + (topScreenTopBarHeight - batteryHeight) / 2;
    screen_draw_texture(batteryIcon, batteryX, batteryY, batteryWidth, batteryHeight);

    u32 wifiIcon = 0;
    u32 wifiStatus = 0;
    if(R_SUCCEEDED(ACU_GetWifiStatus(&wifiStatus)) && wifiStatus) {
        wifiIcon = TEXTURE_WIFI_0 + osGetWifiStrength();
    } else {
        wifiIcon = TEXTURE_WIFI_DISCONNECTED;
    }

    u32 wifiWidth;
    u32 wifiHeight;
    screen_get_texture_size(&wifiWidth, &wifiHeight, wifiIcon);

    float wifiX = topScreenTopBarX + topScreenTopBarWidth - 2 - batteryWidth - 4 - wifiWidth;
    float wifiY = topScreenTopBarY + (topScreenTopBarHeight - wifiHeight) / 2;
    screen_draw_texture(wifiIcon, wifiX, wifiY, wifiWidth, wifiHeight);

    FS_ArchiveResource sd;
    FSUSER_GetSdmcArchiveResource(&sd);

    FS_ArchiveResource nand;
    FSUSER_GetNandArchiveResource(&nand);

    char buffer[64];
    snprintf(buffer, 64, "SD: %.1f MiB, NAND: %.1f MiB", ((u64) sd.freeClusters * (u64) sd.clusterSize) / 1024.0 / 1024.0, ((u64) nand.freeClusters * (u64) nand.clusterSize) / 1024.0 / 1024.0);

    float freeSpaceHeight;
    screen_get_string_size(NULL, &freeSpaceHeight, buffer, 0.5f, 0.5f);

    screen_draw_string(buffer, topScreenBottomBarX + 2, topScreenBottomBarY + (topScreenBottomBarHeight - freeSpaceHeight) / 2, 0.5f, 0.5f, COLOR_TEXT, false);
}

static void ui_draw_bottom(ui_view* ui) {
    u32 bottomScreenBgWidth = 0;
    u32 bottomScreenBgHeight = 0;
    screen_get_texture_size(&bottomScreenBgWidth, &bottomScreenBgHeight, TEXTURE_BOTTOM_SCREEN_BG);

    u32 bottomScreenTopBarWidth = 0;
    u32 bottomScreenTopBarHeight = 0;
    screen_get_texture_size(&bottomScreenTopBarWidth, &bottomScreenTopBarHeight, TEXTURE_BOTTOM_SCREEN_TOP_BAR);

    u32 bottomScreenTopBarShadowWidth = 0;
    u32 bottomScreenTopBarShadowHeight = 0;
    screen_get_texture_size(&bottomScreenTopBarShadowWidth, &bottomScreenTopBarShadowHeight, TEXTURE_BOTTOM_SCREEN_TOP_BAR_SHADOW);

    u32 bottomScreenBottomBarWidth = 0;
    u32 bottomScreenBottomBarHeight = 0;
    screen_get_texture_size(&bottomScreenBottomBarWidth, &bottomScreenBottomBarHeight, TEXTURE_BOTTOM_SCREEN_BOTTOM_BAR);

    u32 bottomScreenBottomBarShadowWidth = 0;
    u32 bottomScreenBottomBarShadowHeight = 0;
    screen_get_texture_size(&bottomScreenBottomBarShadowWidth, &bottomScreenBottomBarShadowHeight, TEXTURE_BOTTOM_SCREEN_BOTTOM_BAR_SHADOW);

    screen_select(GFX_BOTTOM);
    screen_draw_texture(TEXTURE_BOTTOM_SCREEN_BG, (BOTTOM_SCREEN_WIDTH - bottomScreenBgWidth) / 2, (BOTTOM_SCREEN_HEIGHT - bottomScreenBgHeight) / 2, bottomScreenBgWidth, bottomScreenBgHeight);

    if(ui->drawBottom != NULL) {
        ui->drawBottom(ui, ui->data, 0, bottomScreenTopBarHeight, BOTTOM_SCREEN_WIDTH, BOTTOM_SCREEN_HEIGHT - bottomScreenBottomBarHeight);
    }

    float bottomScreenTopBarX = (BOTTOM_SCREEN_WIDTH - bottomScreenTopBarWidth) / 2;
    float bottomScreenTopBarY = 0;
    screen_draw_texture(TEXTURE_BOTTOM_SCREEN_TOP_BAR, bottomScreenTopBarX, bottomScreenTopBarY, bottomScreenTopBarWidth, bottomScreenTopBarHeight);
    screen_draw_texture(TEXTURE_BOTTOM_SCREEN_TOP_BAR_SHADOW, bottomScreenTopBarX, bottomScreenTopBarY + bottomScreenTopBarHeight, bottomScreenTopBarShadowWidth, bottomScreenTopBarShadowHeight);

    float bottomScreenBottomBarX = (BOTTOM_SCREEN_WIDTH - bottomScreenBottomBarWidth) / 2;
    float bottomScreenBottomBarY = BOTTOM_SCREEN_HEIGHT - bottomScreenBottomBarHeight;
    screen_draw_texture(TEXTURE_BOTTOM_SCREEN_BOTTOM_BAR, bottomScreenBottomBarX, bottomScreenBottomBarY, bottomScreenBottomBarWidth, bottomScreenBottomBarHeight);
    screen_draw_texture(TEXTURE_BOTTOM_SCREEN_BOTTOM_BAR_SHADOW, bottomScreenBottomBarX, bottomScreenBottomBarY - bottomScreenBottomBarShadowHeight, bottomScreenBottomBarShadowWidth, bottomScreenBottomBarShadowHeight);

    if(ui->name != NULL) {
        float nameWidth;
        float nameHeight;
        screen_get_string_size(&nameWidth, &nameHeight, ui->name, 0.5f, 0.5f);
        screen_draw_string(ui->name, (BOTTOM_SCREEN_WIDTH - nameWidth) / 2, (bottomScreenTopBarHeight - nameHeight) / 2, 0.5f, 0.5f, COLOR_TEXT, false);
    }

    if(ui->info != NULL) {
        float infoWidth;
        float infoHeight;
        screen_get_string_size(&infoWidth, &infoHeight, ui->info, 0.5f, 0.5f);
        screen_draw_string(ui->info, (BOTTOM_SCREEN_WIDTH - infoWidth) / 2, BOTTOM_SCREEN_HEIGHT - (bottomScreenBottomBarHeight + infoHeight) / 2, 0.5f, 0.5f, COLOR_TEXT, false);
    }
}

bool ui_update() {
    ui_view* ui = NULL;

    hidScanInput();

    ui = ui_top();
    if(ui != NULL && ui->update != NULL) {
        u32 bottomScreenTopBarHeight = 0;
        screen_get_texture_size(NULL, &bottomScreenTopBarHeight, TEXTURE_BOTTOM_SCREEN_TOP_BAR);

        u32 bottomScreenBottomBarHeight = 0;
        screen_get_texture_size(NULL, &bottomScreenBottomBarHeight, TEXTURE_BOTTOM_SCREEN_BOTTOM_BAR);

        ui->update(ui, ui->data, 0, bottomScreenTopBarHeight, BOTTOM_SCREEN_WIDTH, BOTTOM_SCREEN_HEIGHT - bottomScreenBottomBarHeight);
    }

    ui = ui_top();
    if(ui != NULL) {
        screen_begin_frame();
        ui_draw_top(ui);
        ui_draw_bottom(ui);
        screen_end_frame();
    }

    return ui != NULL;
}

void ui_draw_ext_save_data_info(ui_view* view, void* data, float x1, float y1, float x2, float y2) {
    ext_save_data_info* info = (ext_save_data_info*) data;

    char buf[64];

    if(info->hasMeta) {
        u32 metaInfoBoxShadowWidth;
        u32 metaInfoBoxShadowHeight;
        screen_get_texture_size(&metaInfoBoxShadowWidth, &metaInfoBoxShadowHeight, TEXTURE_META_INFO_BOX_SHADOW);

        float metaInfoBoxShadowX = x1 + (x2 - x1 - metaInfoBoxShadowWidth) / 2;
        float metaInfoBoxShadowY = y1 + (y2 - y1) / 4 - metaInfoBoxShadowHeight / 2;
        screen_draw_texture(TEXTURE_META_INFO_BOX_SHADOW, metaInfoBoxShadowX, metaInfoBoxShadowY, metaInfoBoxShadowWidth, metaInfoBoxShadowHeight);

        u32 metaInfoBoxWidth;
        u32 metaInfoBoxHeight;
        screen_get_texture_size(&metaInfoBoxWidth, &metaInfoBoxHeight, TEXTURE_META_INFO_BOX);

        float metaInfoBoxX = x1 + (x2 - x1 - metaInfoBoxWidth) / 2;
        float metaInfoBoxY = y1 + (y2 - y1) / 4 - metaInfoBoxHeight / 2;
        screen_draw_texture(TEXTURE_META_INFO_BOX, metaInfoBoxX, metaInfoBoxY, metaInfoBoxWidth, metaInfoBoxHeight);

        u32 iconWidth;
        u32 iconHeight;
        screen_get_texture_size(&iconWidth, &iconHeight, info->meta.texture);

        float iconX = metaInfoBoxX + (64 - iconWidth) / 2;
        float iconY = metaInfoBoxY + (metaInfoBoxHeight - iconHeight) / 2;
        screen_draw_texture(info->meta.texture, iconX, iconY, iconWidth, iconHeight);

        float shortDescriptionHeight;
        screen_get_string_size(NULL, &shortDescriptionHeight, info->meta.shortDescription, 0.5f, 0.5f);

        float longDescriptionHeight;
        screen_get_string_size(NULL, &longDescriptionHeight, info->meta.longDescription, 0.5f, 0.5f);

        float publisherHeight;
        screen_get_string_size(NULL, &publisherHeight, info->meta.publisher, 0.5f, 0.5f);

        float metaTextX = metaInfoBoxX + 64;

        float shortDescriptionY = metaInfoBoxY + (64 - shortDescriptionHeight - 2 - longDescriptionHeight - 2 - publisherHeight) / 2;
        screen_draw_string(info->meta.shortDescription, metaTextX, shortDescriptionY, 0.5f, 0.5f, COLOR_TEXT, false);

        float longDescriptionY = shortDescriptionY + shortDescriptionHeight + 2;
        screen_draw_string(info->meta.longDescription, metaTextX, longDescriptionY, 0.5f, 0.5f, COLOR_TEXT, false);

        float publisherY = longDescriptionY + longDescriptionHeight + 2;
        screen_draw_string(info->meta.publisher, metaTextX, publisherY, 0.5f, 0.5f, COLOR_TEXT, false);
    }

    snprintf(buf, 64, "Ext Save Data ID: %016llX", info->extSaveDataId);

    float saveDataIdWidth;
    float saveDataIdHeight;
    screen_get_string_size(&saveDataIdWidth, &saveDataIdHeight, buf, 0.5f, 0.5f);

    float saveDataIdX = x1 + (x2 - x1 - saveDataIdWidth) / 2;
    float saveDataIdY = y1 + (y2 - y1) / 2 - 8;
    screen_draw_string(buf, saveDataIdX, saveDataIdY, 0.5f, 0.5f, COLOR_TEXT, false);

    snprintf(buf, 64, "Shared: %s", info->shared ? "Yes" : "No");

    float sharedWidth;
    float sharedHeight;
    screen_get_string_size(&sharedWidth, &sharedHeight, buf, 0.5f, 0.5f);

    float sharedX = x1 + (x2 - x1 - sharedWidth) / 2;
    float sharedY = saveDataIdY + saveDataIdHeight + 2;
    screen_draw_string(buf, sharedX, sharedY, 0.5f, 0.5f, COLOR_TEXT, false);
}

void ui_draw_title_info(ui_view* view, void* data, float x1, float y1, float x2, float y2) {
    title_info* info = (title_info*) data;

    char buf[64];

    if(info->hasMeta) {
        u32 metaInfoBoxShadowWidth;
        u32 metaInfoBoxShadowHeight;
        screen_get_texture_size(&metaInfoBoxShadowWidth, &metaInfoBoxShadowHeight, TEXTURE_META_INFO_BOX_SHADOW);

        float metaInfoBoxShadowX = x1 + (x2 - x1 - metaInfoBoxShadowWidth) / 2;
        float metaInfoBoxShadowY = y1 + (y2 - y1) / 4 - metaInfoBoxShadowHeight / 2;
        screen_draw_texture(TEXTURE_META_INFO_BOX_SHADOW, metaInfoBoxShadowX, metaInfoBoxShadowY, metaInfoBoxShadowWidth, metaInfoBoxShadowHeight);

        u32 metaInfoBoxWidth;
        u32 metaInfoBoxHeight;
        screen_get_texture_size(&metaInfoBoxWidth, &metaInfoBoxHeight, TEXTURE_META_INFO_BOX);

        float metaInfoBoxX = x1 + (x2 - x1 - metaInfoBoxWidth) / 2;
        float metaInfoBoxY = y1 + (y2 - y1) / 4 - metaInfoBoxHeight / 2;
        screen_draw_texture(TEXTURE_META_INFO_BOX, metaInfoBoxX, metaInfoBoxY, metaInfoBoxWidth, metaInfoBoxHeight);

        u32 iconWidth;
        u32 iconHeight;
        screen_get_texture_size(&iconWidth, &iconHeight, info->meta.texture);

        float iconX = metaInfoBoxX + (64 - iconWidth) / 2;
        float iconY = metaInfoBoxY + (metaInfoBoxHeight - iconHeight) / 2;
        screen_draw_texture(info->meta.texture, iconX, iconY, iconWidth, iconHeight);

        float shortDescriptionHeight;
        screen_get_string_size(NULL, &shortDescriptionHeight, info->meta.shortDescription, 0.5f, 0.5f);

        float longDescriptionHeight;
        screen_get_string_size(NULL, &longDescriptionHeight, info->meta.longDescription, 0.5f, 0.5f);

        float publisherHeight;
        screen_get_string_size(NULL, &publisherHeight, info->meta.publisher, 0.5f, 0.5f);

        float metaTextX = metaInfoBoxX + 64;

        float shortDescriptionY = metaInfoBoxY + (64 - shortDescriptionHeight - 2 - longDescriptionHeight - 2 - publisherHeight) / 2;
        screen_draw_string(info->meta.shortDescription, metaTextX, shortDescriptionY, 0.5f, 0.5f, COLOR_TEXT, false);

        float longDescriptionY = shortDescriptionY + shortDescriptionHeight + 2;
        screen_draw_string(info->meta.longDescription, metaTextX, longDescriptionY, 0.5f, 0.5f, COLOR_TEXT, false);

        float publisherY = longDescriptionY + longDescriptionHeight + 2;
        screen_draw_string(info->meta.publisher, metaTextX, publisherY, 0.5f, 0.5f, COLOR_TEXT, false);
    }

    snprintf(buf, 64, "Title ID: %016llX", info->titleId);

    float titleIdWidth;
    float titleIdHeight;
    screen_get_string_size(&titleIdWidth, &titleIdHeight, buf, 0.5f, 0.5f);

    float titleIdX = x1 + (x2 - x1 - titleIdWidth) / 2;
    float titleIdY = y1 + (y2 - y1) / 2 - 8;
    screen_draw_string(buf, titleIdX, titleIdY, 0.5f, 0.5f, COLOR_TEXT, false);

    snprintf(buf, 64, "Media Type: %s", info->mediaType == MEDIATYPE_NAND ? "NAND" : info->mediaType == MEDIATYPE_SD ? "SD" : "Game Card");

    float mediaTypeWidth;
    float mediaTypeHeight;
    screen_get_string_size(&mediaTypeWidth, &mediaTypeHeight, buf, 0.5f, 0.5f);

    float mediaTypeX = x1 + (x2 - x1 - mediaTypeWidth) / 2;
    float mediaTypeY = titleIdY + titleIdHeight + 2;
    screen_draw_string(buf, mediaTypeX, mediaTypeY, 0.5f, 0.5f, COLOR_TEXT, false);

    snprintf(buf, 64, "Product Code: %s", info->productCode);

    float productCodeWidth;
    float productCodeHeight;
    screen_get_string_size(&productCodeWidth, &productCodeHeight, buf, 0.5f, 0.5f);

    float productCodeX = x1 + (x2 - x1 - productCodeWidth) / 2;
    float productCodeY = mediaTypeY + mediaTypeHeight + 2;
    screen_draw_string(buf, productCodeX, productCodeY, 0.5f, 0.5f, COLOR_TEXT, false);

    snprintf(buf, 64, "Region: %s", region_to_string(info->locale->region));

    float regionWidth;
    float regionHeight;
    screen_get_string_size(&regionWidth, &regionHeight, buf, 0.5f, 0.5f);

    float regionX = x1 + (x2 - x1 - regionWidth) / 2;
    float regionY = productCodeY + productCodeHeight + 2;
    screen_draw_string(buf, regionX, regionY, 0.5f, 0.5f, COLOR_TEXT, false);

    snprintf(buf, 64, "Language: %s", language_to_string(info->locale->language));

    float languageWidth;
    float languageHeight;
    screen_get_string_size(&languageWidth, &languageHeight, buf, 0.5f, 0.5f);

    float languageX = x1 + (x2 - x1 - languageWidth) / 2;
    float languageY = regionY + regionHeight + 2;
    screen_draw_string(buf, languageX, languageY, 0.5f, 0.5f, COLOR_TEXT, false);
}
