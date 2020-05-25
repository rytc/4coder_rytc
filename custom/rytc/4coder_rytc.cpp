
static bool rytc_cmd_mode = true;

#include "4coder_default_include.cpp"

CUSTOM_ID(command_map, mapid_cmd);
CUSTOM_ID(colors, defcolor_cursor_cmd);
CUSTOM_ID(colors, defcolor_highlight_cursor_line_cmd);
#include "generated/managed_id_metadata.cpp"

//#include "4coder_default_map.cpp"

BUFFER_HOOK_SIG(rytc_begin_buffer){
    ProfileScope(app, "begin buffer");
    
    Scratch_Block scratch(app);
    
    b32 treat_as_code = false;
    String_Const_u8 file_name = push_buffer_file_name(app, scratch, buffer_id);
    if (file_name.size > 0) {
        String_Const_u8_Array extensions = global_config.code_exts;
        String_Const_u8 ext = string_file_extension(file_name);
        for (i32 i = 0; i < extensions.count; ++i) {
            if (string_match(ext, extensions.strings[i])) {
                
                if (string_match(ext, string_u8_litexpr("cpp")) ||
                    string_match(ext, string_u8_litexpr("h")) ||
                    string_match(ext, string_u8_litexpr("c")) ||
                    string_match(ext, string_u8_litexpr("hpp")) ||
                    string_match(ext, string_u8_litexpr("cc"))){
                    treat_as_code = true;
                }
                
                break;
            }
        }
    }
    
    Command_Map_ID map_id = mapid_cmd;
    Managed_Scope scope = buffer_get_managed_scope(app, buffer_id);
    Command_Map_ID *map_id_ptr = scope_attachment(app, scope, buffer_map_id, Command_Map_ID);
    *map_id_ptr = map_id;
    
    Line_Ending_Kind setting = guess_line_ending_kind_from_buffer(app, buffer_id);
    Line_Ending_Kind *eol_setting = scope_attachment(app, scope, buffer_eol_setting, Line_Ending_Kind);
    *eol_setting = setting;
    
    // NOTE(allen): Decide buffer settings
    b32 wrap_lines = true;
    b32 use_virtual_whitespace = false;
    b32 use_lexer = false;
    if (treat_as_code){
        wrap_lines = global_config.enable_code_wrapping;
        use_virtual_whitespace = global_config.enable_virtual_whitespace;
        use_lexer = true;
    }
    
    String_Const_u8 buffer_name = push_buffer_base_name(app, scratch, buffer_id);
    if (string_match(buffer_name, string_u8_litexpr("*compilation*"))){
        wrap_lines = false;
    }
    
    if (use_lexer){
        ProfileBlock(app, "begin buffer kick off lexer");
        Async_Task *lex_task_ptr = scope_attachment(app, scope, buffer_lex_task, Async_Task);
        *lex_task_ptr = async_task_no_dep(&global_async_system, do_full_lex_async, make_data_struct(&buffer_id));
    }
    
    {
        b32 *wrap_lines_ptr = scope_attachment(app, scope, buffer_wrap_lines, b32);
        *wrap_lines_ptr = wrap_lines;
    }
    
    if (use_virtual_whitespace){
        if (use_lexer){
            buffer_set_layout(app, buffer_id, layout_virt_indent_index_generic);
        }
        else{
            buffer_set_layout(app, buffer_id, layout_virt_indent_literal_generic);
        }
    }
    else{
        buffer_set_layout(app, buffer_id, layout_generic);
    }
    
    // no meaning for return
    return(0);
}

function void
draw_rytc_cursor_mark_highlight(Application_Links *app, View_ID view_id, b32 is_active_view,
                                Buffer_ID buffer, Text_Layout_ID text_layout_id,
                                f32 roundness, f32 outline_thickness){
    b32 has_highlight_range = draw_highlight_range(app, view_id, buffer, text_layout_id, roundness);
    if (!has_highlight_range){
        i64 cursor_pos = view_get_cursor_pos(app, view_id);
        i64 mark_pos = view_get_mark_pos(app, view_id);
        if (is_active_view){
            if(!rytc_cmd_mode)
            {
                draw_character_block(app, text_layout_id, cursor_pos, roundness,
                                     fcolor_id(defcolor_cursor_cmd));
                paint_text_color_pos(app, text_layout_id, cursor_pos,
                                     fcolor_id(defcolor_at_cursor));
                draw_character_wire_frame(app, text_layout_id, mark_pos,
                                          roundness, outline_thickness,
                                          fcolor_id(defcolor_cursor_cmd));
            } else {
                draw_character_block(app, text_layout_id, cursor_pos, roundness,
                                     fcolor_id(defcolor_cursor));
                paint_text_color_pos(app, text_layout_id, cursor_pos,
                                     fcolor_id(defcolor_at_cursor));
                draw_character_wire_frame(app, text_layout_id, mark_pos,
                                          roundness, outline_thickness,
                                          fcolor_id(defcolor_mark));
            }
        }
        else{
            draw_character_wire_frame(app, text_layout_id, mark_pos,
                                      roundness, outline_thickness,
                                      fcolor_id(defcolor_mark));
            draw_character_wire_frame(app, text_layout_id, cursor_pos,
                                      roundness, outline_thickness,
                                      fcolor_id(defcolor_cursor));
        }
    }
}

function void
rytc_draw_scope_highlight(Application_Links *app, Buffer_ID buffer, Text_Layout_ID text_layout_id,
                          i64 pos, ARGB_Color *colors, i32 color_count){
    draw_enclosures(app, text_layout_id, buffer,
                    pos, FindNest_Scope, RangeHighlightKind_LineHighlight,
                    colors, color_count, 0, 0);
}


function void
rytc_render_buffer(Application_Links *app, View_ID view_id, Face_ID face_id,
                   Buffer_ID buffer, Text_Layout_ID text_layout_id,
                   Rect_f32 rect){
    ProfileScope(app, "render buffer");
    
    View_ID active_view = get_active_view(app, Access_Always);
    b32 is_active_view = (active_view == view_id);
    Rect_f32 prev_clip = draw_set_clip(app, rect);
    
    // NOTE(allen): Token colorizing
    Token_Array token_array = get_token_array_from_buffer(app, buffer);
    if (token_array.tokens != 0){
        draw_cpp_token_colors(app, text_layout_id, &token_array);
        
        // NOTE(allen): Scan for TODOs and NOTEs
        if (global_config.use_comment_keyword){
            Comment_Highlight_Pair pairs[] = {
                {string_u8_litexpr("NOTE"), finalize_color(defcolor_comment_pop, 0)},
                {string_u8_litexpr("TODO"), finalize_color(defcolor_comment_pop, 1)},
            };
            draw_comment_highlights(app, buffer, text_layout_id,
                                    &token_array, pairs, ArrayCount(pairs));
        }
    }
    
    else{
        Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
        paint_text_color_fcolor(app, text_layout_id, visible_range, fcolor_id(defcolor_text_default));
    }
    
    i64 cursor_pos = view_correct_cursor(app, view_id);
    view_correct_mark(app, view_id);
    
    // NOTE(allen): Line highlight
    if (global_config.highlight_line_at_cursor && is_active_view){
        if(rytc_cmd_mode) {
            i64 line_number = get_line_number_from_pos(app, buffer, cursor_pos);
            draw_line_highlight(app, text_layout_id, line_number,
                                fcolor_id(defcolor_highlight_cursor_line));
        } else {
            i64 line_number = get_line_number_from_pos(app, buffer, cursor_pos);
            draw_line_highlight(app, text_layout_id, line_number,
                                fcolor_id(defcolor_highlight_cursor_line_cmd));
        }
    }
    
    // NOTE(allen): Scope highlight
    if (global_config.use_scope_highlight){
        Color_Array colors = finalize_color_array(defcolor_back_cycle);
        rytc_draw_scope_highlight(app, buffer, text_layout_id, cursor_pos, colors.vals, colors.count);
    }
    
    if (global_config.use_error_highlight || global_config.use_jump_highlight){
        // NOTE(allen): Error highlight
        String_Const_u8 name = string_u8_litexpr("*compilation*");
        Buffer_ID compilation_buffer = get_buffer_by_name(app, name, Access_Always);
        if (global_config.use_error_highlight){
            draw_jump_highlights(app, buffer, text_layout_id, compilation_buffer,
                                 fcolor_id(defcolor_highlight_junk));
        }
        
        // NOTE(allen): Search highlight
        if (global_config.use_jump_highlight){
            Buffer_ID jump_buffer = get_locked_jump_buffer(app);
            if (jump_buffer != compilation_buffer){
                draw_jump_highlights(app, buffer, text_layout_id, jump_buffer,
                                     fcolor_id(defcolor_highlight_white));
            }
        }
    }
    
    // NOTE(allen): Color parens
    if (global_config.use_paren_helper){
        Color_Array colors = finalize_color_array(defcolor_text_cycle);
        draw_paren_highlight(app, buffer, text_layout_id, cursor_pos, colors.vals, colors.count);
    }
    
    
    
    // NOTE(allen): Cursor shape
    Face_Metrics metrics = get_face_metrics(app, face_id);
    f32 cursor_roundness = (metrics.normal_advance*0.5f)*0.9f;
    f32 mark_thickness = 2.f;
    
    draw_rytc_cursor_mark_highlight(app, view_id, is_active_view, buffer, text_layout_id, cursor_roundness, mark_thickness);
    
    // NOTE(allen): put the actual text on the actual screen
    draw_text_layout_default(app, text_layout_id);
    
    draw_set_clip(app, prev_clip);
}

function void
rytc_render_caller(Application_Links *app, Frame_Info frame_info, View_ID view_id){
    ProfileScope(app, "default render caller");
    View_ID active_view = get_active_view(app, Access_Always);
    b32 is_active_view = (active_view == view_id);
    
    Rect_f32 region = draw_background_and_margin(app, view_id, is_active_view);
    Rect_f32 prev_clip = draw_set_clip(app, region);
    
    Buffer_ID buffer = view_get_buffer(app, view_id, Access_Always);
    Face_ID face_id = get_face_id(app, buffer);
    Face_Metrics face_metrics = get_face_metrics(app, face_id);
    f32 line_height = face_metrics.line_height;
    f32 digit_advance = face_metrics.decimal_digit_advance;
    
    // NOTE(allen): file bar
    b64 showing_file_bar = false;
    if (view_get_setting(app, view_id, ViewSetting_ShowFileBar, &showing_file_bar) && showing_file_bar){
        Rect_f32_Pair pair = layout_file_bar_on_top(region, line_height);
        draw_file_bar(app, view_id, buffer, face_id, pair.min);
        region = pair.max;
    }
    
    Buffer_Scroll scroll = view_get_buffer_scroll(app, view_id);
    
    Buffer_Point_Delta_Result delta = delta_apply(app, view_id,
                                                  frame_info.animation_dt, scroll);
    if (!block_match_struct(&scroll.position, &delta.point)){
        block_copy_struct(&scroll.position, &delta.point);
        view_set_buffer_scroll(app, view_id, scroll, SetBufferScroll_NoCursorChange);
    }
    if (delta.still_animating){
        animate_in_n_milliseconds(app, 0);
    }
    
    // NOTE(allen): query bars
    {
        Query_Bar *space[32];
        Query_Bar_Ptr_Array query_bars = {};
        query_bars.ptrs = space;
        if (get_active_query_bars(app, view_id, ArrayCount(space), &query_bars)){
            for (i32 i = 0; i < query_bars.count; i += 1){
                Rect_f32_Pair pair = layout_query_bar_on_top(region, line_height, 1);
                draw_query_bar(app, query_bars.ptrs[i], face_id, pair.min);
                region = pair.max;
            }
        }
    }
    
    // NOTE(allen): FPS hud
    if (show_fps_hud){
        Rect_f32_Pair pair = layout_fps_hud_on_bottom(region, line_height);
        draw_fps_hud(app, frame_info, face_id, pair.max);
        region = pair.min;
        animate_in_n_milliseconds(app, 1000);
    }
    
    // NOTE(allen): layout line numbers
    Rect_f32 line_number_rect = {};
    if (global_config.show_line_number_margins){
        Rect_f32_Pair pair = layout_line_number_margin(app, buffer, region, digit_advance);
        line_number_rect = pair.min;
        region = pair.max;
    }
    
    // NOTE(allen): begin buffer render
    Buffer_Point buffer_point = scroll.position;
    Text_Layout_ID text_layout_id = text_layout_create(app, buffer, region, buffer_point);
    
    // NOTE(allen): draw line numbers
    if (global_config.show_line_number_margins){
        draw_line_number_margin(app, view_id, buffer, face_id, text_layout_id, line_number_rect);
    }
    
    rytc_render_buffer(app, view_id, face_id, buffer, text_layout_id, region);
    
    text_layout_free(app, text_layout_id);
    draw_set_clip(app, prev_clip);
}

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

static void
rytc_mapping(Mapping *mapping, i64 global_id, i64 file_id, i64 code_id, i64 cmd_id){
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

internal void
rytc_set_hooks(Application_Links *app) {
    set_custom_hook(app, HookID_BufferViewerUpdate, default_view_adjust);
    
    set_custom_hook(app, HookID_ViewEventHandler, default_view_input_handler);
    set_custom_hook(app, HookID_Tick, default_tick);
    set_custom_hook(app, HookID_RenderCaller, rytc_render_caller);
    set_custom_hook(app, HookID_DeltaRule, fixed_time_cubic_delta);
    set_custom_hook_memory_size(app, HookID_DeltaRule,
                                delta_ctx_size(fixed_time_cubic_delta_memory_size));
    set_custom_hook(app, HookID_BufferNameResolver, default_buffer_name_resolution);
    
    set_custom_hook(app, HookID_BeginBuffer, rytc_begin_buffer);
    set_custom_hook(app, HookID_EndBuffer, end_buffer_close_jump_list);
    set_custom_hook(app, HookID_NewFile, default_new_file);
    set_custom_hook(app, HookID_SaveFile, default_file_save);
    set_custom_hook(app, HookID_BufferEditRange, default_buffer_edit_range);
    set_custom_hook(app, HookID_BufferRegion, default_buffer_region);
    
    set_custom_hook(app, HookID_Layout, layout_unwrapped);
    //set_custom_hook(app, HookID_Layout, layout_wrap_anywhere);
    //set_custom_hook(app, HookID_Layout, layout_wrap_whitespace);
    //set_custom_hook(app, HookID_Layout, layout_virt_indent_unwrapped);
    //set_custom_hook(app, HookID_Layout, layout_unwrapped_small_blank_lines);
}

function void rytc_set_default_color_scheme(Application_Links *app);

void
custom_layer_init(Application_Links *app){
    Thread_Context *tctx = get_thread_context(app);
    
    // NOTE(allen): setup for default framework
    //async_task_handler_init(app, &global_async_system);
    //code_index_init();
    //buffer_modified_set_init();
    //Profile_Global_List *list = get_core_profile_list(app);
    //ProfileThreadName(tctx, list, string_u8_litexpr("main"));
    //initialize_managed_id_metadata(app);
    default_framework_init(app);
    rytc_set_default_color_scheme(app);
    
    // NOTE(allen): default hooks and command maps
    //set_all_default_hooks(app);
    rytc_set_hooks(app);
    mapping_init(tctx, &framework_mapping);
    //setup_default_mapping(&framework_mapping, mapid_global, mapid_file, mapid_code);
    rytc_mapping(&framework_mapping, mapid_global, mapid_file, mapid_code, mapid_cmd);
}


function void
rytc_set_default_color_scheme(Application_Links *app){
    if (global_theme_arena.base_allocator == 0){
        global_theme_arena = make_arena_system();
    }
    
    Arena *arena = &global_theme_arena;
    
    default_color_table = make_color_table(app, arena);
    
    default_color_table.arrays[0] = make_colors(arena, 0xFF90B080);
    default_color_table.arrays[defcolor_bar] = make_colors(arena, 0xFF888888);
    default_color_table.arrays[defcolor_base] = make_colors(arena, 0xFF000000);
    default_color_table.arrays[defcolor_pop1] = make_colors(arena, 0xFF3C57DC);
    default_color_table.arrays[defcolor_pop2] = make_colors(arena, 0xFFFF0000);
    default_color_table.arrays[defcolor_back] = make_colors(arena, 0xFF0C0C0C);
    default_color_table.arrays[defcolor_margin] = make_colors(arena, 0xFF181818);
    default_color_table.arrays[defcolor_margin_hover] = make_colors(arena, 0xFF252525);
    default_color_table.arrays[defcolor_margin_active] = make_colors(arena, 0xFF323232);
    default_color_table.arrays[defcolor_list_item] = make_colors(arena, 0xFF181818);
    default_color_table.arrays[defcolor_list_item_hover] = make_colors(arena, 0xFF252525);
    default_color_table.arrays[defcolor_list_item_active] = make_colors(arena, 0xFF323232);
    default_color_table.arrays[defcolor_cursor] = make_colors(arena, 0xFF00EE00);
    default_color_table.arrays[defcolor_cursor_cmd] = make_colors(arena, 0xFFEE0000);
    default_color_table.arrays[defcolor_at_cursor] = make_colors(arena, 0xFF0C0C0C);
    default_color_table.arrays[defcolor_highlight_cursor_line] = make_colors(arena, 0xFF1E1E1E);
    default_color_table.arrays[defcolor_highlight_cursor_line_cmd] = make_colors(arena, 0xFFEE0000);
    default_color_table.arrays[defcolor_highlight] = make_colors(arena, 0xFFDDEE00);
    default_color_table.arrays[defcolor_at_highlight] = make_colors(arena, 0xFFFF44DD);
    default_color_table.arrays[defcolor_mark] = make_colors(arena, 0xFF494949);
    default_color_table.arrays[defcolor_text_default] = make_colors(arena, 0xFF90B080);
    default_color_table.arrays[defcolor_comment] = make_colors(arena, 0xFF2090F0);
    default_color_table.arrays[defcolor_comment_pop] = make_colors(arena, 0xFF00A000, 0xFFA00000);
    default_color_table.arrays[defcolor_keyword] = make_colors(arena, 0xFFD08F20);
    default_color_table.arrays[defcolor_str_constant] = make_colors(arena, 0xFF50FF30);
    default_color_table.arrays[defcolor_char_constant] = make_colors(arena, 0xFF50FF30);
    default_color_table.arrays[defcolor_int_constant] = make_colors(arena, 0xFF50FF30);
    default_color_table.arrays[defcolor_float_constant] = make_colors(arena, 0xFF50FF30);
    default_color_table.arrays[defcolor_bool_constant] = make_colors(arena, 0xFF50FF30);
    default_color_table.arrays[defcolor_preproc] = make_colors(arena, 0xFFA0B8A0);
    default_color_table.arrays[defcolor_include] = make_colors(arena, 0xFF50FF30);
    default_color_table.arrays[defcolor_special_character] = make_colors(arena, 0xFFFF0000);
    default_color_table.arrays[defcolor_ghost_character] = make_colors(arena, 0xFF4E5E46);
    default_color_table.arrays[defcolor_highlight_junk] = make_colors(arena, 0xFF3A0000);
    default_color_table.arrays[defcolor_highlight_white] = make_colors(arena, 0xFF003A3A);
    default_color_table.arrays[defcolor_paste] = make_colors(arena, 0xFFDDEE00);
    default_color_table.arrays[defcolor_undo] = make_colors(arena, 0xFF00DDEE);
    default_color_table.arrays[defcolor_back_cycle] = make_colors(arena, 0xFF130707, 0xFF071307, 0xFF070713, 0xFF131307);
    default_color_table.arrays[defcolor_text_cycle] = make_colors(arena, 0xFFA00000, 0xFF00A000, 0xFF0030B0, 0xFFA0A000);
    default_color_table.arrays[defcolor_line_numbers_back] = make_colors(arena, 0xFF101010);
    default_color_table.arrays[defcolor_line_numbers_text] = make_colors(arena, 0xFF404040);
    
    active_color_table = default_color_table;
}
