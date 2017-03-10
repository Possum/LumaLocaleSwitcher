#pragma once

#include <sys/syslimits.h>
#include <stdio.h>

#include "../../list.h"
#include "../../../locale.h"

typedef struct meta_info_s {
    char shortDescription[0x100];
    char longDescription[0x200];
    char publisher[0x100];
    u32 region;
    u32 texture;
} meta_info;

typedef struct {
    FS_MediaType mediaType;
    u64 titleId;
    char productCode[0x10];
    u16 version;
    u64 installedSize;
    bool twl;
    bool hasMeta;
    meta_info meta;
    Locale* locale;
} title_info;

typedef struct {
    FILE* config_file;
    char path[PATH_MAX];
} config_info; // TODO Why is this defined here?

typedef struct {
    FS_MediaType mediaType;
    u64 titleId;
    u16 version;
} pending_title_info;

typedef struct {
    u64 titleId;
} ticket_info;

typedef struct {
    FS_MediaType mediaType;
    u64 extSaveDataId;
    bool shared;
    bool hasMeta;
    meta_info meta;
} ext_save_data_info;

typedef struct {
    u32 systemSaveDataId;
} system_save_data_info;

typedef struct {
    u64 titleId;
    u16 version;
    u64 installedSize;
    bool hasMeta;
    meta_info meta;
} cia_info;

typedef struct {
    FS_Archive* archive;
    char name[NAME_MAX];
    char path[PATH_MAX];
    bool isDirectory;
    u64 size;

    bool containsCias;
    bool isCia;
    cia_info ciaInfo;

    bool containsTickets;
    bool isTicket;
    ticket_info ticketInfo;
} file_info;

#define R_OUT_OF_MEMORY MAKERESULT(RL_FATAL, RS_OUTOFRESOURCE, RM_APPLICATION, RD_OUT_OF_MEMORY)

typedef enum {
    DATAOP_COPY,
    DATAOP_DELETE
} DataOp;

typedef struct {
    void* data;

    DataOp op;

    // Copy
    bool copyEmpty;

    bool finished;
    bool premature;

    u32 processed;
    u32 total;

    u64 currProcessed;
    u64 currTotal;

    Result (*isSrcDirectory)(void* data, u32 index, bool* isDirectory);
    Result (*makeDstDirectory)(void* data, u32 index);

    Result (*openSrc)(void* data, u32 index, u32* handle);
    Result (*closeSrc)(void* data, u32 index, bool succeeded, u32 handle);

    Result (*getSrcSize)(void* data, u32 handle, u64* size);
    Result (*readSrc)(void* data, u32 handle, u32* bytesRead, void* buffer, u64 offset, u32 size);

    Result (*openDst)(void* data, u32 index, void* initialReadBlock, u32* handle);
    Result (*closeDst)(void* data, u32 index, bool succeeded, u32 handle);

    Result (*writeDst)(void* data, u32 handle, u32* bytesWritten, void* buffer, u64 offset, u32 size);

    // Delete
    Result (*delete)(void* data, u32 index);

    // Errors
    bool (*error)(void* data, u32 index, Result res);
} data_op_info;

bool task_is_quit_all();
void task_quit_all();

Handle task_capture_cam(Handle* mutex, u16* buffer, s16 width, s16 height);

Handle task_data_op(data_op_info* info);

void task_clear_ext_save_data(list_item* items, u32* count);
Handle task_populate_ext_save_data(list_item* items, u32* count, u32 max);

void task_clear_files(list_item* items, u32* count);
Handle task_populate_files(list_item* items, u32* count, u32 max, file_info* dir);

void task_clear_pending_titles(list_item* items, u32* count);
Handle task_populate_pending_titles(list_item* items, u32* count, u32 max);

void task_clear_system_save_data(list_item* items, u32* count);
Handle task_populate_system_save_data(list_item* items, u32* count, u32 max);

void task_clear_tickets(list_item* items, u32* count);
Handle task_populate_tickets(list_item* items, u32* count, u32 max);

void task_clear_titles(list_item* items, u32* count);
Handle task_populate_titles(list_item* items, u32* count, u32 max);
