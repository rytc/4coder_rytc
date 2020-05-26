
static bool rytc_cmd_mode = true;

#include "4coder_default_include.cpp"

CUSTOM_ID(command_map, mapid_cmd);
CUSTOM_ID(colors, defcolor_cursor_cmd);
CUSTOM_ID(colors, defcolor_highlight_cursor_line_cmd);
#include "generated/managed_id_metadata.cpp"


#include "rytc_hooks.cpp"
#include "rytc_commands.cpp"
#include "rytc_mapping.cpp"


void
custom_layer_init(Application_Links *app) 
{
    Thread_Context *tctx = get_thread_context(app);
    
    // NOTE(allen): setup for default framework
    //async_task_handler_init(app, &global_async_system);
    //code_index_init();
    //buffer_modified_set_init();
    //Profile_Global_List *list = get_core_profile_list(app);
    //ProfileThreadName(tctx, list, string_u8_litexpr("main"));
    //initialize_managed_id_metadata(app);
    default_framework_init(app);
    //rytc_set_default_color_scheme(app);
}

// NOTE(allen): default hooks and command maps
//set_all_default_hooks(app);
rytc_set_hooks(app);
mapping_init(tctx, &framework_mapping);
//setup_default_mapping(&framework_mapping, mapid_global, mapid_file, mapid_code);
rytc_mapping(&framework_mapping, mapid_global, mapid_file, mapid_code, mapid_cmd);
}



