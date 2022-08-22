#include <furi.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>
#include "views/spi_mem_manager_read.h"
#include "views/spi_mem_manager_chipinfo.h"
#include "spi_mem_manager_app.h"

#define TAG "SPIMemManager"

uint32_t spi_mem_manager_exit(void* context) {
    UNUSED(context);
    return VIEW_NONE;
}

void spi_mem_manager_submenu_callback(void* context, uint32_t index) {
    SPIMemManager* instance = context;
    if(index == SPIMemManagerSubmenuIndexRead) {
        instance->view_id = SPIMemManagerViewRead;
    }
    if(index == SPIMemManagerSubmenuIndexChipinfo) {
        instance->view_id = SPIMemManagerViewChipinfo;
    }
    view_dispatcher_switch_to_view(instance->view_dispatcher, instance->view_id);
}

void spi_mem_manager_add_submenu_items(SPIMemManager* instance) {
    submenu_add_item(
        instance->submenu,
        "Read",
        SPIMemManagerSubmenuIndexRead,
        spi_mem_manager_submenu_callback,
        instance);
    submenu_add_item(
        instance->submenu,
        "Saved",
        SPIMemManagerSubmenuIndexSaved,
        spi_mem_manager_submenu_callback,
        instance);
    submenu_add_item(
        instance->submenu,
        "Chip info",
        SPIMemManagerSubmenuIndexChipinfo,
        spi_mem_manager_submenu_callback,
        instance);
}

uint32_t spi_mem_manager_show_submenu(void* context) {
    UNUSED(context);
    return SPIMemManagerViewSubmenu;
}

void spi_mem_manager_add_views(SPIMemManager* instance) {
    instance->view_read = spi_mem_manager_read_alloc();
    instance->view_chipinfo = spi_mem_manager_chipinfo_alloc();
    view_set_previous_callback(
        spi_mem_manager_read_get_view(instance->view_read), spi_mem_manager_show_submenu);
    view_set_previous_callback(
        spi_mem_manager_chipinfo_get_view(instance->view_chipinfo), spi_mem_manager_show_submenu);
    view_dispatcher_add_view(
        instance->view_dispatcher,
        SPIMemManagerViewRead,
        spi_mem_manager_read_get_view(instance->view_read));
    view_dispatcher_add_view(
        instance->view_dispatcher,
        SPIMemManagerViewChipinfo,
        spi_mem_manager_chipinfo_get_view(instance->view_chipinfo));
}

SPIMemManager* spi_mem_manager_alloc(void) {
    SPIMemManager* instance = malloc(sizeof(SPIMemManager));
    instance->gui = furi_record_open(RECORD_GUI);
    instance->view_dispatcher = view_dispatcher_alloc();
    instance->submenu = submenu_alloc();
    view_dispatcher_enable_queue(instance->view_dispatcher);
    view_dispatcher_attach_to_gui(
        instance->view_dispatcher, instance->gui, ViewDispatcherTypeFullscreen);
    spi_mem_manager_add_submenu_items(instance);
    spi_mem_manager_add_views(instance);
    view_set_previous_callback(submenu_get_view(instance->submenu), spi_mem_manager_exit);
    view_dispatcher_add_view(
        instance->view_dispatcher, SPIMemManagerViewSubmenu, submenu_get_view(instance->submenu));
    view_set_previous_callback(submenu_get_view(instance->submenu), spi_mem_manager_exit);
    instance->view_id = SPIMemManagerViewSubmenu;
    view_dispatcher_switch_to_view(instance->view_dispatcher, instance->view_id);
    return instance;
}

void spi_mem_manager_free(SPIMemManager* instance) {
    furi_record_close(RECORD_GUI);
    view_dispatcher_remove_view(instance->view_dispatcher, SPIMemManagerViewSubmenu);
    view_dispatcher_remove_view(instance->view_dispatcher, SPIMemManagerViewRead);
    view_dispatcher_remove_view(instance->view_dispatcher, SPIMemManagerViewChipinfo);
    view_dispatcher_free(instance->view_dispatcher);
    submenu_free(instance->submenu);
    spi_mem_manager_read_free(instance->view_read);
    spi_mem_manager_chipinfo_free(instance->view_chipinfo);
    free(instance);
}

int32_t spi_mem_manager_app(void* p) {
    UNUSED(p);
    SPIMemManager* SPIMemManagerApp = spi_mem_manager_alloc();
    view_dispatcher_run(SPIMemManagerApp->view_dispatcher);
    spi_mem_manager_free(SPIMemManagerApp);
    return 0;
}
