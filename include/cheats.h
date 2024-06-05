#include "structs.h"
#include <dbglogger.h>
#define LOG dbglogger_log

#define CHEATSMGR_PATH              "/data/CheatsManager/"
#define CHEATSMGR_LOCAL_CACHE       CHEATSMGR_PATH "temp/"
#define CHEATSMGR_UPDATE_URL        "https://api.github.com/repos/bucanero/PS4CheatsManager/releases/latest"

#ifndef DEBUG_ENABLE_LOG
#define CHEATSMGR_APP_PATH          "/data/CheatsManager/debug/"
#define CHEATSMGR_SANDBOX_PATH      "/mnt/sandbox/LOAD00044_000%s/"
#else
#define CHEATSMGR_APP_PATH          "/mnt/sandbox/CHTM00777_000/app0/assets/"
#define CHEATSMGR_SANDBOX_PATH      "/mnt/sandbox/CHTM00777_000%s/"
#endif

#define GOLDHEN_PATH                "/data/GoldHEN"
#define GOLDCHEATS_DATA_PATH        GOLDHEN_PATH "/cheats/"
#define GOLDCHEATS_PATCH_PATH       GOLDHEN_PATH "/patches/"
#define GOLDCHEATS_PLUGINS_PATH     GOLDHEN_PATH "/plugins/"
#define GOLDPATCH_SETTINGS_PATH     GOLDCHEATS_PATCH_PATH "settings/"

#define GOLDCHEATS_BACKUP_PREFIX    "GH-cheats"
#define GOLDPATCH_BACKUP_PREFIX     "GH-patches"
#define GOLDPLUGINS_BACKUP_PREFIX   "GH-plugins"

#define LOCAL_TEMP_ZIP              "appdata.zip"
#define GOLDCHEATS_URL              "https://github.com/GoldHEN/GoldHEN_Cheat_Repository/archive/refs/heads/"
#define GOLDCHEATS_FILE             "main.zip"
#define GOLDPATCH_URL               "https://github.com/illusion0001/PS4-PS5-Game-Patch/releases/latest/download/"
#define GOLDPATCH_FILE              "patch1.zip"
#define GOLDPLUGINS_UPDATE_URL      "https://api.github.com/repos/GoldHEN/GoldHEN_Plugins_Repository/releases/latest"

#define MAX_USB_DEVICES         8
#define USB0_PATH               "/mnt/usb0/"
#define USB1_PATH               "/mnt/usb1/"
#define USB_PATH                "/mnt/usb%d/"

#define ONLINE_URL				"https://goldhen.github.io/GoldHEN_Cheat_Repository/"
#define ONLINE_CACHE_TIMEOUT    24*3600     // 1-day local cache


enum cmd_code_enum
{
    CMD_CODE_NULL,

// Code commands
    CMD_TOGGLE_PATCH,
    CMD_VIEW_RAW_PATCH,
    CMD_VIEW_DETAILS,
    CMD_DECRYPT_MC4,

// Remove commands
    CMD_REMOVE_CHEATS,
    CMD_REMOVE_PATCHES,
    CMD_REMOVE_PLUGINS,

// Update commands
    CMD_UPD_INTERNET_CHEATS,
    CMD_UPD_INTERNET_PATCHES,
    CMD_UPD_INTERNET_PLUGINS,
    CMD_UPD_LOCAL_CHEATS,
    CMD_UPD_LOCAL_PATCHES,
    CMD_UPD_LOCAL_PLUGINS,

// Backup commands
    CMD_BACKUP_CHEATS_USB,
    CMD_BACKUP_CHEATS_HDD,
    CMD_BACKUP_PATCHES_USB,
    CMD_BACKUP_PATCHES_HDD,
    CMD_BACKUP_PLUGINS_USB,
    CMD_BACKUP_PLUGINS_HDD,
};

// Save flags
#define CHEAT_FLAG_LOCKED        1
#define CHEAT_FLAG_OWNER         2
#define CHEAT_FLAG_JSON          4
#define CHEAT_FLAG_PS1           8
#define CHEAT_FLAG_PS2           16
#define CHEAT_FLAG_MC4           32
#define CHEAT_FLAG_SHN           64
#define CHEAT_FLAG_PATCH         128
#define CHEAT_FLAG_ONLINE        256
#define CHEAT_FLAG_PS4           512
#define CHEAT_FLAG_HDD           1024

enum save_type_enum
{
    FILE_TYPE_NULL,
    TYPE_UPDATE_INTERNET,
    TYPE_UPDATE_LOCAL,
    FILE_TYPE_MENU,
    FILE_TYPE_PS4,
};

enum char_flag_enum
{
    CHAR_TAG_NULL,
    CHAR_TAG_PS1,
    CHAR_TAG_PS2,
    CHAR_TAG_JSON,
    CHAR_TAG_SHN,
    CHAR_TAG_MC4,
    CHAR_TAG_APPLY,
    CHAR_TAG_OWNER,
    CHAR_TAG_LOCKED,
    CHAR_RES_TAB,
    CHAR_RES_LF,
    CHAR_TAG_TRANSFER,
    CHAR_TAG_ZIP,
    CHAR_RES_CR,
    CHAR_TAG_PCE,
    CHAR_TAG_WARNING,
    CHAR_BTN_X,
    CHAR_BTN_S,
    CHAR_BTN_T,
    CHAR_BTN_O,
    CHAR_TRP_BRONZE,
    CHAR_TRP_SILVER,
    CHAR_TRP_GOLD,
    CHAR_TRP_PLATINUM,
    CHAR_TRP_SYNC,
    CHAR_TAG_PS4,
};

enum code_type_enum
{
    PATCH_NULL,
    PATCH_VIEW,
    PATCH_COMMAND,
};

enum save_sort_enum
{
    SORT_DISABLED,
    SORT_BY_NAME,
    SORT_BY_TITLE_ID,
};

typedef struct game_entry
{
    char * name;
    char * title_id;
    char * version;
    char * path;
    uint16_t flags;
    uint16_t type;
    list_t * codes;
} game_entry_t;

typedef struct
{
    list_t * list;
    char path[128];
    char* title;
    uint8_t icon_id;
    size_t filtered;
    void (*UpdatePath)(char *);
    int (*ReadCodes)(game_entry_t *);
    list_t* (*ReadList)(const char*);
} game_list_t;

list_t * ReadUsbList(const char* userPath);
list_t * ReadUserList(const char* userPath);
list_t * ReadOnlineList(const char* urlPath);
list_t * ReadBackupList(const char* userPath);
list_t * ReadPatchList(const char* userPath);
void UnloadGameList(list_t * list);
char * readTextFile(const char * path, long* size);
int sortGameList_Exists(const void* A, const void* B);
int sortGameList_Compare(const void* A, const void* B);
int sortGameList_Compare_TitleID(const void* a, const void* b);
int sortCodeList_Compare(const void* A, const void* B);
int ReadCodes(game_entry_t * save);
int ReadPatches(game_entry_t * game);
int ReadOnlineCodes(game_entry_t * game);
int ReadBackupCodes(game_entry_t * bup);

int http_init(void);
void http_end(void);
int http_download(const char* url, const char* filename, const char* local_dst, int show_progress);

int extract_zip(const char* zip_file, const char* dest_path);
int zip_directory(const char* basedir, const char* inputdir, const char* output_zipfile);
int extract_zip_gh(const char* zip_file, const char* dest_path);

int show_dialog(int dialog_type, const char * format, ...);
void init_progress_bar(const char* msg);
void update_progress_bar(uint64_t progress, const uint64_t total_size, const char* msg);
void end_progress_bar(void);
#define show_message(...)	show_dialog(0, __VA_ARGS__)

int init_loading_screen(const char* msg);
void stop_loading_screen();

void execCodeCommand(code_entry_t* code, const char* codecmd);
uint64_t patch_hash_calc(const game_entry_t* game, const code_entry_t* code);
char* mc4_decrypt(const char* data);

int get_save_details(const game_entry_t *save, char** details);
int orbis_SaveUmount(const char* mountPath);
int orbis_UpdateSaveParams(const char* mountPath, const char* title, const char* subtitle, const char* details);

#define startsWith(str1, str2) (strncmp(str1, str2, strlen(str2))==0)
