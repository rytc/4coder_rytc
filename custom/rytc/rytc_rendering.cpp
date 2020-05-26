

// Borrowed from https://github.com/ryanfleury/4coder_fleury/blob/master/4coder_fleury_brace.cpp
static void
Fleury4RenderBraceLines(Application_Links *app, Buffer_ID buffer, View_ID view,
                        Text_Layout_ID text_layout_id, i64 pos)
{
    ProfileScope(app, "[Fleury] Brace Lines");
    
    Face_ID face_id = get_face_id(app, buffer);
    Token_Array token_array = get_token_array_from_buffer(app, buffer);
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    
    if (token_array.tokens != 0)
    {
        Token_Iterator_Array it = token_iterator_pos(0, &token_array, pos);
        Token *token = token_it_read(&it);
        if(token != 0 && token->kind == TokenBaseKind_ScopeOpen)
        {
            pos = token->pos + token->size;
        }
        else
        {
            
            if(token_it_dec_all(&it))
            {
                token = token_it_read(&it);
                
                if (token->kind == TokenBaseKind_ScopeClose &&
                    pos == token->pos + token->size)
                {
                    pos = token->pos;
                }
                
            }
            
        }
        
    }
    
    Face_Metrics metrics = get_face_metrics(app, face_id);
    
    Scratch_Block scratch(app);
    Range_i64_Array ranges = get_enclosure_ranges(app, scratch, buffer, pos, RangeHighlightKind_CharacterHighlight);
    float x_position = view_get_screen_rect(app, view).x0 + 4 -
        view_get_buffer_scroll(app, view).position.pixel_shift.x;
    
    for (i32 i = ranges.count - 1; i >= 0; i -= 1)
    {
        Range_i64 range = ranges.ranges[i];
        
        Rect_f32 range_start_rect = text_layout_character_on_screen(app, text_layout_id, range.start);
        Rect_f32 range_end_rect = text_layout_character_on_screen(app, text_layout_id, range.end);
        
        float y_start = 0;
        float y_end = 10000;
        
        if(range.start >= visible_range.start)
        {
            y_start = range_start_rect.y0 + metrics.line_height;
        }
        if(range.end <= visible_range.end)
        {
            y_end = range_end_rect.y0;
        }
        
        Rect_f32 line_rect = {0};
        line_rect.x0 = x_position;
        line_rect.x1 = x_position+1;
        line_rect.y0 = y_start;
        line_rect.y1 = y_end;
        u32 color = finalize_color(defcolor_comment, 0);
        color &= 0x00ffffff;
        color |= 0x60000000;
        draw_rectangle(app, line_rect, 0.5f, color);
        
        x_position += metrics.space_advance * global_config.virtual_whitespace_regular_indent;
        
    }
    
}
