#include <unistd.h>
#include <string.h>
#include <math.h>

#include "orbisPad.h"
#include "cheats.h"
#include "common.h"
#include "menu.h"
#include "menu_gui.h"
#include "libfont.h"
#include "ttf_render.h"

extern game_list_t hdd_cheats;
extern game_list_t hdd_patches;
extern game_list_t online_cheats;
extern game_list_t update_cheats;

extern int close_app;

int menu_options_maxopt = 0;
int *menu_options_maxsel;

int menu_id = 0;												// Menu currently in
int menu_sel = 0;												// Index of selected item (use varies per menu)
int menu_old_sel[TOTAL_MENU_IDS] = { 0 };						// Previous menu_sel for each menu
int last_menu_id[TOTAL_MENU_IDS] = { 0 };						// Last menu id called (for returning)

game_entry_t* selected_entry;
code_entry_t* selected_centry;
int option_index = 0;


void initMenuOptions(void)
{
	menu_options_maxopt = 0;
	while (menu_options[menu_options_maxopt].name)
		menu_options_maxopt++;
	
	menu_options_maxsel = (int *)calloc(1, menu_options_maxopt * sizeof(int));
	
	for (int i = 0; i < menu_options_maxopt; i++)
	{
		menu_options_maxsel[i] = 0;
		if (menu_options[i].type == APP_OPTION_LIST)
		{
			while (menu_options[i].options[menu_options_maxsel[i]])
				menu_options_maxsel[i]++;
		}
	}
}

static int ReloadUserGames(game_list_t* save_list, const char* message)
{
	init_loading_screen(message);

	if (save_list->list)
	{
		UnloadGameList(save_list->list);
		save_list->list = NULL;
	}

	if (save_list->UpdatePath)
		save_list->UpdatePath(save_list->path);

	save_list->list = save_list->ReadList(save_list->path);
	if (gcm_config.doSort == SORT_BY_NAME)
		list_bubbleSort(save_list->list, &sortGameList_Compare);
	else if (gcm_config.doSort == SORT_BY_TITLE_ID)
		list_bubbleSort(save_list->list, &sortGameList_Compare_TitleID);

	stop_loading_screen();

	if (!save_list->list && save_list->icon_id != header_ico_xmb_png_index)
	{
		show_message("No data found");
		return 0;
	}

	return list_count(save_list->list);
}

static code_entry_t* LoadRawPatch(void)
{
	size_t len;
	char patchPath[256];
	code_entry_t* centry = calloc(1, sizeof(code_entry_t));

	centry->name = strdup(selected_entry->title_id);
	snprintf(patchPath, sizeof(patchPath), GOLDCHEATS_DATA_PATH "%s.savepatch", selected_entry->title_id);
	read_buffer(patchPath, (u8**) &centry->codes, &len);
	centry->codes[len] = 0;

	return centry;
}

static code_entry_t* LoadSaveDetails(void)
{
	code_entry_t* centry = calloc(1, sizeof(code_entry_t));
	centry->name = strdup(selected_entry->title_id);

	if (!get_save_details(selected_entry, &centry->codes))
		asprintf(&centry->codes, "Error getting details (%s)", selected_entry->name);

	LOG("%s", centry->codes);
	return (centry);
}

static void SetMenu(int id)
{   
	switch (menu_id) //Leaving menu
	{
		case MENU_MAIN_SCREEN: //Main Menu
		case MENU_HDD_CHEATS: //HHD Saves Menu
		case MENU_HDD_PATCHES: //HHD Saves Menu
		case MENU_ONLINE_DB: //Cheats Online Menu
		case MENU_UPDATE_CHEATS: //Backup Menu
//			menu_textures[icon_png_file_index].size = 0;
			break;

		case MENU_SETTINGS: //Options Menu
		case MENU_CREDITS: //About Menu
		case MENU_PATCHES: //Cheat Selection Menu
			break;

		case MENU_SAVE_DETAILS:
		case MENU_PATCH_VIEW: //Cheat View Menu
			if (gcm_config.doAni)
				Draw_CheatsMenu_View_Ani_Exit();
			break;

		case MENU_CODE_OPTIONS: //Cheat Option Menu
			if (gcm_config.doAni)
				Draw_CheatsMenu_Options_Ani_Exit();
			break;
	}
	
	switch (id) //going to menu
	{
		case MENU_MAIN_SCREEN: //Main Menu
			if (gcm_config.doAni || menu_id == MENU_MAIN_SCREEN) //if load animation
				Draw_MainMenu_Ani();
			break;

		case MENU_HDD_CHEATS: //HDD saves Menu
			if (!hdd_cheats.list && !ReloadUserGames(&hdd_cheats, "Loading Game Cheats..."))
				return;
			
			if (gcm_config.doAni)
				Draw_UserCheatsMenu_Ani(&hdd_cheats);
			break;

		case MENU_HDD_PATCHES: //HDD patches Menu
			if(!hdd_patches.list && file_exists(GOLDCHEATS_PLUGINS_PATH "game_patch.prx") != SUCCESS)
			{
				show_message(
					"Game Patch Plugin is not installed!\n"
					"You can install the plugin from\n\n"
					"https://github.com/GoldHEN/GoldHEN_Plugins_Repository");
			}
			if (!hdd_patches.list && !ReloadUserGames(&hdd_patches, "Loading Game Patches..."))
				return;
			
			if (gcm_config.doAni)
				Draw_UserCheatsMenu_Ani(&hdd_patches);
			break;

		case MENU_ONLINE_DB: //Cheats Online Menu
			if (!online_cheats.list && !ReloadUserGames(&online_cheats, "Loading Online Database..."))
				return;

			if (gcm_config.doAni)
				Draw_UserCheatsMenu_Ani(&online_cheats);
			break;

		case MENU_CREDITS: //About Menu
			// set to display the PSID on the About menu
			if (gcm_config.doAni)
				Draw_AboutMenu_Ani();
			break;

		case MENU_SETTINGS: //Options Menu
			if (gcm_config.doAni)
				Draw_OptionsMenu_Ani();
			break;

		case MENU_UPDATE_CHEATS: //User Backup Menu
			if (!update_cheats.list && !ReloadUserGames(&update_cheats, ""))
				return;

			if (gcm_config.doAni)
				Draw_UserCheatsMenu_Ani(&update_cheats);
			break;

		case MENU_PATCHES: //Cheat Selection Menu
			//if entering from game list, don't keep index, otherwise keep
			if (menu_id == MENU_HDD_CHEATS || menu_id == MENU_ONLINE_DB || menu_id == MENU_HDD_PATCHES || menu_id == MENU_UPDATE_CHEATS)
				menu_old_sel[MENU_PATCHES] = 0;
/*
			char iconfile[256];
			snprintf(iconfile, sizeof(iconfile), "%s" "sce_sys/icon0.png", selected_entry->path);

			if (selected_entry->flags & CHEAT_FLAG_ONLINE)
			{
				snprintf(iconfile, sizeof(iconfile), CHEATSMGR_LOCAL_CACHE "%s.PNG", selected_entry->title_id);

				if (file_exists(iconfile) != SUCCESS)
					http_download(selected_entry->path, "icon0.png", iconfile, 0);
			}
			else if (selected_entry->flags & CHEAT_FLAG_HDD)
				snprintf(iconfile, sizeof(iconfile), PS4_SAVES_PATH_HDD "%s/%s_icon0.png", gcm_config.user_id, selected_entry->title_id, selected_entry->version);

			if (file_exists(iconfile) == SUCCESS)
				LoadFileTexture(iconfile, icon_png_file_index);
			else
				menu_textures[icon_png_file_index].size = 0;
*/
			if (gcm_config.doAni && menu_id != MENU_PATCH_VIEW && menu_id != MENU_CODE_OPTIONS)
				Draw_CheatsMenu_Selection_Ani();
			break;

		case MENU_PATCH_VIEW: //Cheat View Menu
			menu_old_sel[MENU_PATCH_VIEW] = 0;
			if (gcm_config.doAni)
				Draw_CheatsMenu_View_Ani("Code view");
			break;

		case MENU_SAVE_DETAILS: //Save Detail View Menu
			if (gcm_config.doAni)
				Draw_CheatsMenu_View_Ani(selected_entry->name);
			break;

		case MENU_CODE_OPTIONS: //Cheat Option Menu
			menu_old_sel[MENU_CODE_OPTIONS] = 0;
			if (gcm_config.doAni)
				Draw_CheatsMenu_Options_Ani();
			break;
	}
	
	menu_old_sel[menu_id] = menu_sel;
	if (last_menu_id[menu_id] != id)
		last_menu_id[id] = menu_id;
	menu_id = id;
	
	menu_sel = menu_old_sel[menu_id];
}

static void move_selection_back(int game_count, int steps)
{
	menu_sel -= steps;
	if ((menu_sel == -1) && (steps == 1))
		menu_sel = game_count - 1;
	else if (menu_sel < 0)
		menu_sel = 0;
}

static void move_selection_fwd(int game_count, int steps)
{
	menu_sel += steps;
	if ((menu_sel == game_count) && (steps == 1))
		menu_sel = 0;
	else if (menu_sel >= game_count)
		menu_sel = game_count - 1;
}

static void doSaveMenu(game_list_t * save_list)
{
	if(orbisPadGetButtonHold(ORBIS_PAD_BUTTON_UP))
		move_selection_back(list_count(save_list->list), 1);

	else if(orbisPadGetButtonHold(ORBIS_PAD_BUTTON_DOWN))
		move_selection_fwd(list_count(save_list->list), 1);

	else if (orbisPadGetButtonHold(ORBIS_PAD_BUTTON_LEFT))
		move_selection_back(list_count(save_list->list), 5);

	else if (orbisPadGetButtonHold(ORBIS_PAD_BUTTON_L1))
		move_selection_back(list_count(save_list->list), 25);

	else if (orbisPadGetButtonHold(ORBIS_PAD_BUTTON_L2))
		menu_sel = 0;

	else if (orbisPadGetButtonHold(ORBIS_PAD_BUTTON_RIGHT))
		move_selection_fwd(list_count(save_list->list), 5);

	else if (orbisPadGetButtonHold(ORBIS_PAD_BUTTON_R1))
		move_selection_fwd(list_count(save_list->list), 25);

	else if (orbisPadGetButtonHold(ORBIS_PAD_BUTTON_R2))
		menu_sel = list_count(save_list->list) - 1;

	else if (orbisPadGetButtonPressed(ORBIS_PAD_BUTTON_CIRCLE))
	{
		SetMenu(MENU_MAIN_SCREEN);
		return;
	}
	else if (orbisPadGetButtonPressed(ORBIS_PAD_BUTTON_CROSS))
	{
		selected_entry = list_get_item(save_list->list, menu_sel);

		if (!selected_entry->codes && !save_list->ReadCodes(selected_entry))
		{
			show_message("No data found in folder:\n%s", selected_entry->path);
			return;
		}

		if (gcm_config.doSort != SORT_DISABLED)
//				((save_list->icon_id == cat_bup_png_index) || (save_list->icon_id == cat_db_png_index)))
			list_bubbleSort(selected_entry->codes, &sortCodeList_Compare);

		SetMenu(MENU_PATCHES);
		return;
	}
	else if (orbisPadGetButtonPressed(ORBIS_PAD_BUTTON_TRIANGLE)) // && save_list->UpdatePath)
	{
		menu_sel = 0;
		if (save_list->filtered)
		{
			save_list->list->count = save_list->filtered;
			save_list->filtered = 0;

			if (gcm_config.doSort == SORT_BY_NAME)
				list_bubbleSort(save_list->list, &sortGameList_Compare);
			else if (gcm_config.doSort == SORT_BY_TITLE_ID)
				list_bubbleSort(save_list->list, &sortGameList_Compare_TitleID);
		}
		else
		{
			save_list->filtered = save_list->list->count;
			list_bubbleSort(save_list->list, &sortGameList_Exists);

			game_entry_t* item;
			save_list->list->count = 0;
			for (list_node_t *node = list_head(save_list->list); (item = list_get(node)); node = list_next(node))
				if (item->flags & CHEAT_FLAG_OWNER) save_list->list->count++;
		}
	}
	else if (orbisPadGetButtonPressed(ORBIS_PAD_BUTTON_SQUARE))
	{
		ReloadUserGames(save_list, "Reloading List...");
	}

	Draw_UserCheatsMenu(save_list, menu_sel, 0xFF);
}

static void doMainMenu(void)
{
	// Check the pads.
	if(orbisPadGetButtonHold(ORBIS_PAD_BUTTON_LEFT))
		move_selection_back(MENU_CREDITS, 1);

	else if(orbisPadGetButtonHold(ORBIS_PAD_BUTTON_RIGHT))
		move_selection_fwd(MENU_CREDITS, 1);

	else if (orbisPadGetButtonPressed(ORBIS_PAD_BUTTON_CROSS))
	{
		SetMenu(menu_sel+1);
		drawScene();
		return;
	}

	else if(orbisPadGetButtonPressed(ORBIS_PAD_BUTTON_CIRCLE))
	{
		close_app = 1;
	}
	
	Draw_MainMenu();
}

static void doAboutMenu(void)
{
	static int ll = 0;

	// Check the pads.
	if (orbisPadGetButtonPressed(ORBIS_PAD_BUTTON_CIRCLE))
	{
		if (ll)
			gcm_config.music = (ll & 0x01);

		ll = 0;
		SetMenu(MENU_MAIN_SCREEN);
		return;
	}
	else if (orbisPadGetButtonPressed(ORBIS_PAD_BUTTON_TOUCH_PAD))
	{
		ll = (0x02 | gcm_config.music);
		gcm_config.music = 1;
	}

	Draw_AboutMenu(ll);
}

static void doOptionsMenu(void)
{
	// Check the pads.
	if(orbisPadGetButtonHold(ORBIS_PAD_BUTTON_UP))
		move_selection_back(menu_options_maxopt, 1);

	else if(orbisPadGetButtonHold(ORBIS_PAD_BUTTON_DOWN))
		move_selection_fwd(menu_options_maxopt, 1);

	else if (orbisPadGetButtonPressed(ORBIS_PAD_BUTTON_CIRCLE))
	{
		save_app_settings(&gcm_config);
		set_ttf_window(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WIN_SKIP_LF);
		SetMenu(MENU_MAIN_SCREEN);
		return;
	}
	else if (orbisPadGetButtonHold(ORBIS_PAD_BUTTON_LEFT))
	{
		if (menu_options[menu_sel].type == APP_OPTION_LIST)
		{
			if (*menu_options[menu_sel].value > 0)
				(*menu_options[menu_sel].value)--;
			else
				*menu_options[menu_sel].value = menu_options_maxsel[menu_sel] - 1;
		}
		else if (menu_options[menu_sel].type == APP_OPTION_INC)
			(*menu_options[menu_sel].value)--;
		
		if (menu_options[menu_sel].type != APP_OPTION_CALL)
			menu_options[menu_sel].callback(*menu_options[menu_sel].value);
	}
	else if (orbisPadGetButtonHold(ORBIS_PAD_BUTTON_RIGHT))
	{
		if (menu_options[menu_sel].type == APP_OPTION_LIST)
		{
			if (*menu_options[menu_sel].value < (menu_options_maxsel[menu_sel] - 1))
				*menu_options[menu_sel].value += 1;
			else
				*menu_options[menu_sel].value = 0;
		}
		else if (menu_options[menu_sel].type == APP_OPTION_INC)
			*menu_options[menu_sel].value += 1;

		if (menu_options[menu_sel].type != APP_OPTION_CALL)
			menu_options[menu_sel].callback(*menu_options[menu_sel].value);
	}
	else if (orbisPadGetButtonPressed(ORBIS_PAD_BUTTON_CROSS))
	{
		if (menu_options[menu_sel].type == APP_OPTION_BOOL)
			menu_options[menu_sel].callback(*menu_options[menu_sel].value);

		else if (menu_options[menu_sel].type == APP_OPTION_CALL)
			menu_options[menu_sel].callback(0);
	}
	
	Draw_OptionsMenu();
}

static int count_code_lines(void)
{
	//Calc max
	int max = 0;
	const char * str;

	for(str = selected_centry->codes; *str; ++str)
		max += (*str == '\n');

	if (max <= 0)
		max = 1;

	return max;
}

static void doPatchViewMenu(void)
{
	int max = count_code_lines();
	
	// Check the pads.
	if(orbisPadGetButtonHold(ORBIS_PAD_BUTTON_UP))
		move_selection_back(max, 1);

	else if(orbisPadGetButtonHold(ORBIS_PAD_BUTTON_DOWN))
		move_selection_fwd(max, 1);

	else if (orbisPadGetButtonPressed(ORBIS_PAD_BUTTON_CIRCLE))
	{
		SetMenu(last_menu_id[MENU_PATCH_VIEW]);
		return;
	}
	
	Draw_CheatsMenu_View("Code view");
}

static void doCodeOptionsMenu(void)
{
    code_entry_t* code = list_get_item(selected_entry->codes, menu_old_sel[last_menu_id[MENU_CODE_OPTIONS]]);
	// Check the pads.
	if(orbisPadGetButtonHold(ORBIS_PAD_BUTTON_UP))
		move_selection_back(selected_centry->options[option_index].size, 1);

	else if(orbisPadGetButtonHold(ORBIS_PAD_BUTTON_DOWN))
		move_selection_fwd(selected_centry->options[option_index].size, 1);

	else if (orbisPadGetButtonPressed(ORBIS_PAD_BUTTON_CIRCLE))
	{
		code->activated = 0;
		SetMenu(last_menu_id[MENU_CODE_OPTIONS]);
		return;
	}
	else if (orbisPadGetButtonPressed(ORBIS_PAD_BUTTON_CROSS))
	{
		code->options[option_index].sel = menu_sel;

		if (code->type == PATCH_COMMAND)
			execCodeCommand(code, code->options[option_index].value[menu_sel]);

		option_index++;
		
		if (option_index >= code->options_count)
		{
			SetMenu(last_menu_id[MENU_CODE_OPTIONS]);
			return;
		}
		else
			menu_sel = 0;
	}
	
	Draw_CheatsMenu_Options();
}

static void doSaveDetailsMenu(void)
{
	int max = count_code_lines();

	// Check the pads.
	if(orbisPadGetButtonHold(ORBIS_PAD_BUTTON_UP))
		move_selection_back(max, 1);

	else if(orbisPadGetButtonHold(ORBIS_PAD_BUTTON_DOWN))
		move_selection_fwd(max, 1);

	if (orbisPadGetButtonPressed(ORBIS_PAD_BUTTON_CIRCLE))
	{
		if (selected_centry->name)
			free(selected_centry->name);
		if (selected_centry->codes)
			free(selected_centry->codes);
		free(selected_centry);

		SetMenu(last_menu_id[MENU_SAVE_DETAILS]);
		return;
	}
	
	Draw_CheatsMenu_View(selected_entry->name);
}

static void doPatchMenu(void)
{
	// Check the pads.
	if(orbisPadGetButtonHold(ORBIS_PAD_BUTTON_UP))
		move_selection_back(list_count(selected_entry->codes), 1);

	else if(orbisPadGetButtonHold(ORBIS_PAD_BUTTON_DOWN))
		move_selection_fwd(list_count(selected_entry->codes), 1);

	else if (orbisPadGetButtonHold(ORBIS_PAD_BUTTON_LEFT))
		move_selection_back(list_count(selected_entry->codes), 5);

	else if (orbisPadGetButtonHold(ORBIS_PAD_BUTTON_RIGHT))
		move_selection_fwd(list_count(selected_entry->codes), 5);

	else if (orbisPadGetButtonHold(ORBIS_PAD_BUTTON_L1))
		move_selection_back(list_count(selected_entry->codes), 25);

	else if (orbisPadGetButtonHold(ORBIS_PAD_BUTTON_R1))
		move_selection_fwd(list_count(selected_entry->codes), 25);

	else if (orbisPadGetButtonPressed(ORBIS_PAD_BUTTON_CIRCLE))
	{
		SetMenu(last_menu_id[MENU_PATCHES]);
		return;
	}
	else if (orbisPadGetButtonPressed(ORBIS_PAD_BUTTON_CROSS))
	{
		selected_centry = list_get_item(selected_entry->codes, menu_sel);

		if (selected_entry->flags & CHEAT_FLAG_PATCH && selected_centry->type != PATCH_NULL)
		{
			char code = CMD_TOGGLE_PATCH;
			selected_centry->activated = !selected_centry->activated;
			execCodeCommand(selected_centry, &code);
		}

		if (selected_centry->type == PATCH_COMMAND)
			execCodeCommand(selected_centry, selected_centry->codes);

		if (selected_centry->activated)
		{
				/*
				// Only activate Required codes if a cheat is selected
				if (selected_centry->type == PATCH_GAMEGENIE || selected_centry->type == PATCH_BSD)
				{
					code_entry_t* code;
					list_node_t* node;

					for (node = list_head(selected_entry->codes); (code = list_get(node)); node = list_next(node))
						if (wildcard_match_icase(code->name, "*(REQUIRED)*"))
							code->activated = 1;
				}

				if (!selected_centry->options)
				{
					int size;
					selected_entry->codes[menu_sel].options = ReadOptions(selected_entry->codes[menu_sel], &size);
					selected_entry->codes[menu_sel].options_count = size;
				}
				*/
			
			if (selected_centry->options)
			{
				option_index = 0;
				SetMenu(MENU_CODE_OPTIONS);
				return;
			}

			if (selected_centry->codes[0] == CMD_VIEW_RAW_PATCH)
			{
				selected_centry->activated = 0;
				selected_centry = LoadRawPatch();
				SetMenu(MENU_SAVE_DETAILS);
				return;
			}

			if (selected_centry->codes[0] == CMD_VIEW_DETAILS)
			{
				selected_centry->activated = 0;
				selected_centry = LoadSaveDetails();
				SetMenu(MENU_SAVE_DETAILS);
				return;
			}
		}
	}
	else if (orbisPadGetButtonPressed(ORBIS_PAD_BUTTON_TRIANGLE))
	{
		selected_centry = list_get_item(selected_entry->codes, menu_sel);

		if (selected_centry->type == PATCH_VIEW)
		{
			SetMenu(MENU_PATCH_VIEW);
			return;
		}
	}
	
	Draw_CheatsMenu_Selection(menu_sel, 0xFFFFFFFF);
}

// Resets new frame
void drawScene(void)
{
	switch (menu_id)
	{
		case MENU_MAIN_SCREEN:
			doMainMenu();
			break;

		case MENU_HDD_CHEATS: //HDD Saves Menu
			doSaveMenu(&hdd_cheats);
			break;

		case MENU_HDD_PATCHES: //HDD Patches Menu
			doSaveMenu(&hdd_patches);
			break;

		case MENU_ONLINE_DB: //Online Cheats Menu
			doSaveMenu(&online_cheats);
			break;

		case MENU_CREDITS: //About Menu
			doAboutMenu();
			break;

		case MENU_SETTINGS: //Options Menu
			doOptionsMenu();
			break;

		case MENU_UPDATE_CHEATS: //User Backup Menu
			doSaveMenu(&update_cheats);
			break;

		case MENU_PATCHES: //Cheats Selection Menu
			doPatchMenu();
			break;

		case MENU_PATCH_VIEW: //Cheat View Menu
			doPatchViewMenu();
			break;

		case MENU_CODE_OPTIONS: //Cheat Option Menu
			doCodeOptionsMenu();
			break;

		case MENU_SAVE_DETAILS: //Save Details Menu
			doSaveDetailsMenu();
			break;
	}
}
