
static void
rytc_mapping(Mapping *mapping, i64 global_id, i64 file_id, i64 code_id, i64 cmd_id) {
    MappingScope();
    SelectMapping(mapping);
    
    SelectMap(global_id);
    {
    	BindCore(default_startup, CoreCode_Startup);
        BindCore(default_try_exit, CoreCode_TryExit);
        Bind(move_up, KeyCode_Up);
        Bind(move_down, KeyCode_Down);
        Bind(move_left, KeyCode_Left);
        Bind(move_right, KeyCode_Right);
        Bind(page_up, KeyCode_PageUp);
        Bind(page_down, KeyCode_PageDown);
        Bind(if_read_only_goto_position,  KeyCode_Return);
        
        Bind(project_fkey_command, KeyCode_F1);
        Bind(project_fkey_command, KeyCode_F2);
        Bind(project_fkey_command, KeyCode_F3);
        Bind(project_fkey_command, KeyCode_F4);
        Bind(project_fkey_command, KeyCode_F5);
        Bind(project_fkey_command, KeyCode_F6);
        Bind(project_fkey_command, KeyCode_F7);
        Bind(project_fkey_command, KeyCode_F8);
        Bind(project_fkey_command, KeyCode_F9);
        Bind(project_fkey_command, KeyCode_F10);
        Bind(project_fkey_command, KeyCode_F11);
        Bind(project_fkey_command, KeyCode_F12);
        Bind(project_fkey_command, KeyCode_F13);
        Bind(project_fkey_command, KeyCode_F14);
        Bind(project_fkey_command, KeyCode_F15);
        Bind(project_fkey_command, KeyCode_F16);
        
        Bind(change_active_panel, KeyCode_Tab, KeyCode_Control);
        
        BindMouseWheel(mouse_wheel_scroll);
        BindMouseWheel(mouse_wheel_change_face_size, KeyCode_Control);
        BindMouse(click_set_cursor_and_mark, MouseCode_Left);
        BindMouseRelease(click_set_cursor, MouseCode_Left);
        BindCore(click_set_cursor_and_mark, CoreCode_ClickActivateView);
        BindMouseMove(click_set_cursor_if_lbutton);
        
        Bind(seek_beginning_of_line, KeyCode_Home);
        Bind(seek_end_of_line, KeyCode_End);
        
        Bind(kill_buffer, KeyCode_Q, KeyCode_Control);
        
        Bind(delete_char, KeyCode_Delete);
        
        Bind(theme_lister, KeyCode_T, KeyCode_Control);
    }
    
    SelectMap(code_id);
    {
        ParentMap(global_id);
        BindTextInput(write_text_input);
        Bind(rytc_set_mode_cmd, KeyCode_Escape);
        Bind(backspace_char, KeyCode_Backspace);
    }
    
    
    
    SelectMap(cmd_id);
    {
        ParentMap(global_id);
        Bind(move_up,               KeyCode_K);
        Bind(page_up,               KeyCode_K, KeyCode_Control);
        Bind(move_up_to_blank_line, KeyCode_K, KeyCode_Shift);
        Bind(move_line_up, KeyCode_K, KeyCode_Control, KeyCode_Shift);
        
        Bind(move_down,               KeyCode_J);
        Bind(page_down,               KeyCode_J, KeyCode_Control);
        Bind(move_down_to_blank_line, KeyCode_J, KeyCode_Shift);
        Bind(move_line_down, KeyCode_J, KeyCode_Control, KeyCode_Shift);
        
        
        Bind(move_left,               KeyCode_H);
        Bind(seek_beginning_of_line, KeyCode_H, KeyCode_Control);
        
        Bind(rytc_append, KeyCode_A);
        Bind(move_right,       KeyCode_L);
        Bind(seek_end_of_line, KeyCode_L, KeyCode_Control);
        
        Bind(move_right_whitespace_or_token_boundary, KeyCode_W);
        Bind(move_right_alpha_numeric_or_camel_boundary, KeyCode_E);
        Bind(move_left_alpha_numeric_or_camel_boundary, KeyCode_B);
        
        Bind(copy, KeyCode_Y);
        Bind(paste, KeyCode_P);
        Bind(undo, KeyCode_U);
        Bind(redo, KeyCode_R);
        
        Bind(delete_char, KeyCode_X);
        Bind(cut, KeyCode_X, KeyCode_Control);
        Bind(delete_range, KeyCode_X, KeyCode_Shift);
        
        
        Bind(delete_line, KeyCode_D);
        Bind(delete_alpha_numeric_boundary, KeyCode_D, KeyCode_Control);
        
        Bind(goto_next_jump, KeyCode_RightBracket);
        Bind(goto_prev_jump, KeyCode_LeftBracket);
        Bind(goto_first_jump, KeyCode_RightBracket, KeyCode_Control);
        
        Bind(rytc_set_mode_code, KeyCode_I);
        
        Bind(interactive_new, KeyCode_N);
        Bind(rytc_new_line, KeyCode_O);
        Bind(interactive_open, KeyCode_O, KeyCode_Control);
        Bind(interactive_switch_buffer, KeyCode_Tick);
        Bind(rytc_switch_buffer_other, KeyCode_Tick, KeyCode_Control);
        
        // @TODO eol nixify
        Bind(save_all_dirty_buffers, KeyCode_S);
        
        Bind(search, KeyCode_F);
        Bind(list_all_substring_locations_case_insensitive, KeyCode_F, KeyCode_Shift);
        Bind(query_replace, KeyCode_F, KeyCode_Control);
        Bind(goto_line, KeyCode_G);
        
        Bind(set_mark, KeyCode_V);
        Bind(cursor_mark_swap, KeyCode_V, KeyCode_Control);
        
        Bind(list_all_functions_current_buffer, KeyCode_Q);
    }
    
    SelectMap(file_id);
    {
        ParentMap(cmd_id);
        BindTextInput(write_text_input);
    }
}

