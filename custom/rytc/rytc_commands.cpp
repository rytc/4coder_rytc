


static void
rytc_set_mode_cmd_impl(Application_Links* app) {
    if(!rytc_cmd_mode) {
        Buffer_ID bufferID = view_get_buffer(app, get_active_view(app, Access_ReadVisible), Access_ReadVisible);
        Managed_Scope scope = buffer_get_managed_scope(app, bufferID);
        Command_Map_ID* map_id_ptr = scope_attachment(app, scope, buffer_map_id, Command_Map_ID);
        *map_id_ptr = mapid_cmd;
        rytc_cmd_mode = true;
    }
}

static void
rytc_set_mode_code_impl(Application_Links* app) {
    if(rytc_cmd_mode) {
        Buffer_ID bufferID = view_get_buffer(app, get_active_view(app, Access_ReadVisible), Access_ReadVisible);
        Managed_Scope scope = buffer_get_managed_scope(app, bufferID);
        Command_Map_ID* map_id_ptr = scope_attachment(app, scope, buffer_map_id, Command_Map_ID);
        *map_id_ptr = mapid_code;
        rytc_cmd_mode = false;
    }
}

static void
rytc_toggle_mode(Application_Links *app) {
    if(!rytc_cmd_mode) {
    	rytc_set_mode_cmd_impl(app);
    } else {
        rytc_set_mode_code_impl(app);
    }
}

CUSTOM_COMMAND_SIG(rytc_set_mode_cmd) {
    rytc_set_mode_cmd_impl(app);
}

CUSTOM_COMMAND_SIG(rytc_set_mode_code) {
    rytc_set_mode_code_impl(app);
}

CUSTOM_COMMAND_SIG(rytc_switch_buffer_other) {
    change_active_panel_send_command(app, interactive_open_or_new);
}

CUSTOM_COMMAND_SIG(rytc_new_line) {
    seek_pos_of_visual_line(app, Side_Max);
    write_string(app, string_u8_litexpr("\n"));
    rytc_set_mode_code_impl(app);
}

CUSTOM_COMMAND_SIG(rytc_append) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    view_set_cursor_by_character_delta(app, view, 1);
    no_mark_snap_to_cursor_if_shift(app, view);
    rytc_set_mode_code_impl(app);
}
