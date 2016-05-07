#pragma once

#include "../task/task.h"

void action_change_region(title_info* info, bool* populated);
void action_change_language(title_info* info, bool* populated);
void action_use_system_default(title_info* info, bool* populated);
void action_delete_dir(file_info* info, bool* populated);
void action_change_locale_dir(config_info* info, bool* populated);
void action_pick_locale_dir(config_info* info, bool* populated);
