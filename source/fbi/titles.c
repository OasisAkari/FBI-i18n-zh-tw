#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <3ds.h>

#include "resources.h"
#include "section.h"
#include "action/action.h"
#include "task/uitask.h"
#include "../core/core.h"

static list_item launch_title = {"啓動程式", COLOR_TEXT, action_launch_title};
static list_item delete_title = {"刪除程式", COLOR_TEXT, action_delete_title};
static list_item delete_title_ticket = {"刪除程式和保存檔案", COLOR_TEXT, action_delete_title_ticket};
static list_item extract_smdh = {"提取SMDH", COLOR_TEXT, action_extract_smdh};
static list_item import_seed = {"導入Seed", COLOR_TEXT, action_import_seed};
static list_item browse_save_data = {"瀏覽存檔", COLOR_TEXT, action_browse_title_save_data};
static list_item import_save_data = {"導入存檔", COLOR_TEXT, action_import_twl_save};
static list_item export_save_data = {"導出存檔", COLOR_TEXT, action_export_twl_save};
static list_item erase_save_data = {"清除存檔", COLOR_TEXT, action_erase_twl_save};
static list_item import_secure_value = {"導入安全値", COLOR_TEXT, action_import_secure_value};
static list_item export_secure_value = {"導出安全値", COLOR_TEXT, action_export_secure_value};
static list_item delete_secure_value = {"刪除安全値", COLOR_TEXT, action_delete_secure_value};

typedef struct {
    populate_titles_data populateData;

    bool showGameCard;
    bool showSD;
    bool showNAND;
    bool sortById;
    bool sortByName;
    bool sortBySize;

    bool populated;
} titles_data;

typedef struct {
    linked_list* items;
    list_item* selected;
} titles_action_data;

static void titles_action_draw_top(ui_view* view, void* data, float x1, float y1, float x2, float y2, list_item* selected) {
    task_draw_title_info(view, ((titles_action_data*) data)->selected->data, x1, y1, x2, y2);
}

static void titles_action_update(ui_view* view, void* data, linked_list* items, list_item* selected, bool selectedTouched) {
    titles_action_data* actionData = (titles_action_data*) data;

    if(hidKeysDown() & KEY_B) {
        ui_pop();
        list_destroy(view);

        free(data);

        return;
    }

    if(selected != NULL && selected->data != NULL && (selectedTouched || (hidKeysDown() & KEY_A))) {
        void(*action)(linked_list*, list_item*) = (void(*)(linked_list*, list_item*)) selected->data;

        ui_pop();
        list_destroy(view);

        action(actionData->items, actionData->selected);

        free(data);

        return;
    }

    if(linked_list_size(items) == 0) {
        linked_list_add(items, &launch_title);

        title_info* info = (title_info*) actionData->selected->data;

        if(info->mediaType != MEDIATYPE_GAME_CARD) {
            linked_list_add(items, &delete_title);
            linked_list_add(items, &delete_title_ticket);
        }

        if(!info->twl) {
            linked_list_add(items, &extract_smdh);

            if(info->mediaType != MEDIATYPE_GAME_CARD) {
                linked_list_add(items, &import_seed);
            }

            linked_list_add(items, &browse_save_data);

            if(info->mediaType != MEDIATYPE_GAME_CARD) {
                linked_list_add(items, &import_secure_value);
                linked_list_add(items, &export_secure_value);
                linked_list_add(items, &delete_secure_value);
            }
        } else if(info->mediaType == MEDIATYPE_GAME_CARD) {
            linked_list_add(items, &import_save_data);
            linked_list_add(items, &export_save_data);
            linked_list_add(items, &erase_save_data);
        }
    }
}

static void titles_action_open(linked_list* items, list_item* selected) {
    titles_action_data* data = (titles_action_data*) calloc(1, sizeof(titles_action_data));
    if(data == NULL) {
        error_display(NULL, NULL, "無法分配程式操作數據");

        return;
    }

    data->items = items;
    data->selected = selected;

    list_display("應用操作", "A: 選擇, B: 返回", data, titles_action_update, titles_action_draw_top);
}

static void titles_options_add_entry(linked_list* items, const char* name, bool* val) {
    list_item* item = (list_item*) calloc(1, sizeof(list_item));
    if(item != NULL) {
        snprintf(item->name, LIST_ITEM_NAME_MAX, "%s", name);
        item->color = *val ? COLOR_ENABLED : COLOR_DISABLED;
        item->data = val;

        linked_list_add(items, item);
    }
}

static void titles_options_update(ui_view* view, void* data, linked_list* items, list_item* selected, bool selectedTouched) {
    titles_data* listData = (titles_data*) data;

    if(hidKeysDown() & KEY_B) {
        linked_list_iter iter;
        linked_list_iterate(items, &iter);

        while(linked_list_iter_has_next(&iter)) {
            free(linked_list_iter_next(&iter));
            linked_list_iter_remove(&iter);
        }

        ui_pop();
        list_destroy(view);

        return;
    }

    if(selected != NULL && selected->data != NULL && (selectedTouched || (hidKeysDown() & KEY_A))) {
        bool* val = (bool*) selected->data;
        *val = !(*val);

        if(*val && (val == &listData->sortById || val == &listData->sortByName || val == &listData->sortBySize)) {
            if(val == &listData->sortById) {
                listData->sortByName = false;
                listData->sortBySize = false;
            } else if(val == &listData->sortByName) {
                listData->sortById = false;
                listData->sortBySize = false;
            } else if(val == &listData->sortBySize) {
                listData->sortById = false;
                listData->sortByName = false;
            }

            linked_list_iter iter;
            linked_list_iterate(items, &iter);
            while(linked_list_iter_has_next(&iter)) {
                list_item* item = (list_item*) linked_list_iter_next(&iter);

                item->color = *(bool*) item->data ? COLOR_ENABLED : COLOR_DISABLED;
            }
        } else {
            selected->color = *val ? COLOR_ENABLED : COLOR_DISABLED;
        }

        listData->populated = false;
    }

    if(linked_list_size(items) == 0) {
        titles_options_add_entry(items, "顯示遊戲Card", &listData->showGameCard);
        titles_options_add_entry(items, "顯示SD Card", &listData->showSD);
        titles_options_add_entry(items, "顯示NAND", &listData->showNAND);
        titles_options_add_entry(items, "按ID排列", &listData->sortById);
        titles_options_add_entry(items, "按名稱排列", &listData->sortByName);
        titles_options_add_entry(items, "按大小排列", &listData->sortBySize);
    }
}

static void titles_options_open(titles_data* data) {
    list_display("選項", "A: 切換, B: 返回", data, titles_options_update, NULL);
}

static void titles_draw_top(ui_view* view, void* data, float x1, float y1, float x2, float y2, list_item* selected) {
    if(selected != NULL && selected->data != NULL) {
        task_draw_title_info(view, selected->data, x1, y1, x2, y2);
    }
}

static void titles_update(ui_view* view, void* data, linked_list* items, list_item* selected, bool selectedTouched) {
    titles_data* listData = (titles_data*) data;

    if(hidKeysDown() & KEY_B) {
        if(!listData->populateData.finished) {
            svcSignalEvent(listData->populateData.cancelEvent);
            while(!listData->populateData.finished) {
                svcSleepThread(1000000);
            }
        }

        ui_pop();

        task_clear_titles(items);
        list_destroy(view);

        free(listData);
        return;
    }

    if(hidKeysDown() & KEY_SELECT) {
        titles_options_open(listData);
        return;
    }

    if(!listData->populated || (hidKeysDown() & KEY_X)) {
        if(!listData->populateData.finished) {
            svcSignalEvent(listData->populateData.cancelEvent);
            while(!listData->populateData.finished) {
                svcSleepThread(1000000);
            }
        }

        listData->populateData.items = items;
        Result res = task_populate_titles(&listData->populateData);
        if(R_FAILED(res)) {
            error_display_res(NULL, NULL, res, "無法初始化程式列表");
        }

        listData->populated = true;
    }

    if(listData->populateData.finished && R_FAILED(listData->populateData.result)) {
        error_display_res(NULL, NULL, listData->populateData.result, "無法填充程式列表");

        listData->populateData.result = 0;
    }

    if(selected != NULL && selected->data != NULL && (selectedTouched || (hidKeysDown() & KEY_A))) {
        titles_action_open(items, selected);
        return;
    }
}

static bool titles_filter(void* data, u64 titleId, FS_MediaType mediaType) {
    titles_data* listData = (titles_data*) data;

    if(mediaType == MEDIATYPE_GAME_CARD) {
        return listData->showGameCard;
    } else if(mediaType == MEDIATYPE_SD) {
        return listData->showSD;
    } else {
        return listData->showNAND;
    }
}

static int titles_compare(void* data, const void* p1, const void* p2) {
    titles_data* listData = (titles_data*) data;

    list_item* info1 = (list_item*) p1;
    list_item* info2 = (list_item*) p2;

    title_info* title1 = (title_info*) info1->data;
    title_info* title2 = (title_info*) info2->data;


    if(title1->mediaType > title2->mediaType) {
        return -1;
    } else if(title1->mediaType < title2->mediaType) {
        return 1;
    } else {
        if(!title1->twl && title2->twl) {
            return -1;
        } else if(title1->twl && !title2->twl) {
            return 1;
        } else {
            if(listData->sortById) {
                u64 id1 = title1->titleId;
                u64 id2 = title2->titleId;

                return id1 > id2 ? 1 : id1 < id2 ? -1 : 0;
            } else if(listData->sortByName) {
                bool title1HasName = title1->hasMeta && !string_is_empty(title1->meta.shortDescription);
                bool title2HasName = title2->hasMeta && !string_is_empty(title2->meta.shortDescription);

                if(title1HasName && !title2HasName) {
                    return -1;
                } else if(!title1HasName && title2HasName) {
                    return 1;
                } else {
                    return strncasecmp(info1->name, info2->name, sizeof(info1->name));
                }
            } else if(listData->sortBySize) {
                u64 size1 = title1->installedSize;
                u64 size2 = title2->installedSize;

                return size1 > size2 ? -1 : size1 < size2 ? 1 : 0;
            } else {
                return 0;
            }
        }
    }
}

void titles_open() {
    titles_data* data = (titles_data*) calloc(1, sizeof(titles_data));
    if(data == NULL) {
        error_display(NULL, NULL, "無法分配程式數據.");

        return;
    }

    data->populateData.userData = data;
    data->populateData.filter = titles_filter;
    data->populateData.compare = titles_compare;

    data->populateData.finished = true;

    data->showGameCard = true;
    data->showSD = true;
    data->showNAND = true;
    data->sortById = false;
    data->sortByName = true;
    data->sortBySize = false;

    list_display("程式（Titles）", "A: 選擇, B: 返回, X: 刷新, Select: 選項", data, titles_update, titles_draw_top);
}

// オケー