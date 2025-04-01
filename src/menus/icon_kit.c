#include "main.h"
#include "icon_kit.h"
#include "level_select.h"
#include "memory.h"
#include "level_routines.h"

void upload_icons(u32 gamemode, u32 page);
void draw_icons(u32 gamemode, u32 page);
void draw_selected_icon(u32 gamemode);
void draw_selected_glyph(u32 selected_x, u32 selected_y);
void update_gamemode_palette(u32 gamemode);


const u16 num_icons[GAMEMODE_COUNT] = {
    ICON_COUNT_CUBES,
    ICON_COUNT_SHIPS,
    ICON_COUNT_BALLS,
    ICON_COUNT_UFOS,
    ICON_COUNT_WAVES
};

void icon_kit_loop() {
    // Enable BG 0
    REG_DISPCNT = DCNT_MODE0 | DCNT_OBJ | DCNT_OBJ_1D | DCNT_BG0;

    REG_BG0CNT  = BG_CBB(0) | BG_SBB(26) | BG_REG_32x32 | BG_PRIO(3);
    REG_BG0HOFS = 0;
    REG_BG0VOFS = 0;

    memset32(shadow_oam, ATTR0_HIDE, 256);
    memset16(rotation_buffer, 0x0000, NUM_ROT_SLOTS);

    memset32(palette_buffer, 0, 256);
    memcpy16(palette_buffer, icon_kit_palette, sizeof(icon_kit_palette) / sizeof(COLOR));

    memcpy32(&tile_mem_obj[0][1008], &icon_kit_chr[ICON_KIT_SELECTION_INDEX], sizeof(TILE) / 4 * 16);

    memcpy32(&se_mem[26][0], icon_kit_l0_tilemap, sizeof(icon_kit_l0_tilemap) / sizeof(u32));
    memcpy32(&tile_mem[0][0], icon_kit_chr, sizeof(icon_kit_chr) / 8);

    nextSpr = 0;

    set_player_colors(palette_buffer, CLR_WHITE, CLR_GRAY, CLR_WHITE);
    set_player_colors(&palette_buffer[0x10], save_data.p1_col_selected, save_data.p2_col_selected, save_data.glow_col_selected);


    u32 selected_gamemode = GAMEMODE_CUBE;
    s32 selected_page = save_data.cube_selected / ICONS_PER_PAGE;
    s32 selected_x = save_data.cube_selected % ICONS_COLUMNS;
    s32 selected_y = (save_data.cube_selected / ICONS_COLUMNS) % ICONS_ROWS;

    // Draw the cube page
    upload_icons(GAMEMODE_CUBE, selected_page);
    draw_selected_icon(GAMEMODE_CUBE);
    draw_icons(GAMEMODE_CUBE, selected_page);
    draw_selected_glyph(selected_x, selected_y);
    update_gamemode_palette(GAMEMODE_CUBE);
    
    obj_copy(oam_mem, shadow_oam, 128);
    obj_aff_copy(obj_aff_mem, obj_aff_buffer, 32);

    fade_in();

    while (1) {
        key_poll();
        
        nextSpr = 0;
        obj_copy(oam_mem, shadow_oam, 128);
        obj_aff_copy(obj_aff_mem, obj_aff_buffer, 32);
        
        memset32(shadow_oam, ATTR0_HIDE, 256);

        // Title screen
        if (key_hit(KEY_B)) {
            game_state = STATE_TITLE_SCREEN;
            
            // Save icons
            write_save_block();

            fade_out();
            break;
        }

        // Decrement selected gamemode
        if (key_hit(KEY_L)) {
            // If we aren't on the first gamemode, switch pages
            if (selected_gamemode > 0) {
                selected_gamemode--;

                // Select the saved icon
                selected_page = *icon_selection_table[selected_gamemode] / ICONS_PER_PAGE;
                selected_x = *icon_selection_table[selected_gamemode] % ICONS_COLUMNS;
                selected_y = (*icon_selection_table[selected_gamemode] / ICONS_COLUMNS) % ICONS_ROWS;
                upload_icons(selected_gamemode, selected_page);
                update_gamemode_palette(selected_gamemode);
            }
        }

        // Increment selected gamemode
        if (key_hit(KEY_R)) {
            // If we aren't on the last gamemode, switch pages
            if (selected_gamemode < GAMEMODE_COUNT - 1) {
                selected_gamemode++;

                // Select the saved icon
                selected_page = *icon_selection_table[selected_gamemode] / ICONS_PER_PAGE;
                selected_x = *icon_selection_table[selected_gamemode] % ICONS_COLUMNS;
                selected_y = (*icon_selection_table[selected_gamemode] / ICONS_COLUMNS) % ICONS_ROWS;
                upload_icons(selected_gamemode, selected_page);
                update_gamemode_palette(selected_gamemode);
            }
        }

        // Key UP
        if(key_hit(KEY_UP)) {
            selected_y -= 1;

            // Keep y pos positive (in bounds)
            if (selected_y < 0) {
                selected_y = 0;
            } else {
                *icon_selection_table[selected_gamemode] -= ICONS_COLUMNS;
            }
        }

        // Key DOWN
        if(key_hit(KEY_DOWN)) {
            selected_y += 1;

            // Keep y pos in bounds
            if (selected_y >= ICONS_ROWS) {
                selected_y = ICONS_ROWS - 1;
            } else {
                // If moving down would make the selection be out of bounds, don't move
                s32 selection = *icon_selection_table[selected_gamemode] + ICONS_COLUMNS;
                if (selection >= num_icons[selected_gamemode]) selected_y -= 1;
                else *icon_selection_table[selected_gamemode] = selection;
            }
        }

        // Key LEFT
        if(key_hit(KEY_LEFT)) {
            selected_x -= 1;
            // Check if we switched pages by moving left on the first column
            if (selected_x < 0) {
                // If the selected page is not the first one, switch pages
                if (selected_page > 0) {
                    selected_x = ICONS_COLUMNS - 1;
                    selected_page--;
                    *icon_selection_table[selected_gamemode] -= (ICONS_PER_PAGE / 2) + 1;
                    upload_icons(selected_gamemode, selected_page);
                } else {
                    // Else keep in bounds
                    selected_x = 0;
                }
            } else {
                *icon_selection_table[selected_gamemode] -= 1;
            }
        }

        // Key RIGHT
        if(key_hit(KEY_RIGHT)) {
            selected_x += 1;
            // Check if we switched pages by moving right on the last column
            if (selected_x >= ICONS_COLUMNS) {
                // If the selected page not the last one, switch pages
                if (selected_page < (num_icons[selected_gamemode] / ICONS_PER_PAGE)) {
                    selected_x = 0;
                    selected_page++;
                    s32 selection = *icon_selection_table[selected_gamemode] + (ICONS_PER_PAGE / 2) + 1;
                    
                    // If the page switch would make the selection be out of bounds, set it to the first icon on the page
                    if (selection >= num_icons[selected_gamemode]) {
                        // Reset y to 0
                        selected_y = 0;

                        // Go up one row
                        *icon_selection_table[selected_gamemode] = selection - (ICONS_PER_PAGE / 2);
                    } else *icon_selection_table[selected_gamemode] = selection;

                    upload_icons(selected_gamemode, selected_page);
                } else {
                    // Else keep in bounds
                    selected_x = ICONS_COLUMNS - 1;
                }
            } else {  
                // If moving right would make the selection be out of bounds, don't move
                s32 selection = *icon_selection_table[selected_gamemode] + 1;
                if (selection >= num_icons[selected_gamemode]) selected_x -= 1;
                else *icon_selection_table[selected_gamemode] = selection;
            }
        }

        // Draw icons
        draw_icons(selected_gamemode, selected_page);
        draw_selected_icon(selected_gamemode);
        draw_selected_glyph(selected_x, selected_y);

        // Wait for VSYNC
        VBlankIntrWait();
    }
}

void upload_icons(u32 gamemode, u32 page) {
    for (s32 icon = 0; icon < ICONS_PER_PAGE; icon++) {
        u32 icon_index = icon + (page * ICONS_PER_PAGE);

        // Get lower nybble, this is inside the row of icons
        u32 lower = (icon_index & 0b111) << 1;

        // Get the rest of bits, shift twice to get the proper even 16 tile line
        u32 higher = (icon_index & ~0b111) << 2;

        u32 index = higher | lower;

        // Copy player sprite into VRAM
        memcpy32(&tile_mem_obj[0][(icon << 2) + VRAM_ICON_OFFSET], &icon_kit[gamemode][index], PLAYER_CHR_SIZE);
        memcpy32(&tile_mem_obj[0][(icon << 2) + 2 + VRAM_ICON_OFFSET], &icon_kit[gamemode][index + 0x10], PLAYER_CHR_SIZE);
    }
}

void draw_icons(u32 gamemode, u32 page) {
    u32 x_pos = ICON_X;
    u32 y_pos = ICON_Y;
    s32 icon = 0;
    
    for (s32 y = 0; y < ICONS_ROWS; y++) {
        for (s32 x = 0; x < ICONS_COLUMNS; x++) {
            // Draw icon following a grid pattern
            oam_metaspr(x_pos + (x * ICON_STEP), y_pos + (y * ICON_STEP), iconKitIcon, FALSE, FALSE, (icon << 2) + VRAM_ICON_OFFSET, 0, 0, 0, TRUE);

            icon++;

            // Exit if we ran out of icons to draw (only on last page)
            if (icon + (page * ICONS_PER_PAGE) >= num_icons[gamemode]) return;
        }
    }
}

void draw_selected_icon(u32 gamemode) {
    upload_player_chr(gamemode, ID_PLAYER_1);
    oam_metaspr(SELECTED_ICON_X, SELECTED_ICON_Y, player1Spr, FALSE, FALSE, 0, 1, 0, 0, TRUE);   
    obj_aff_scale_inv(&obj_aff_buffer[0], float2fx(2.0), float2fx(2.0));
}

void draw_selected_glyph(u32 selected_x, u32 selected_y) {
    oam_metaspr(ICON_X + (selected_x * ICON_STEP) - 8, ICON_Y + (selected_y * ICON_STEP) - 8, iconKitSelection, FALSE, FALSE, 0, 0, 0, 0, TRUE);
}

void set_tab_palette(u32 pal, u32 x, u32 y);

void update_gamemode_palette(u32 gamemode) {
    u32 x = ICON_TAB_X;
    u32 y = ICON_TAB_Y;

    for (u32 gamemode_tab = 0; gamemode_tab < GAMEMODE_COUNT; gamemode_tab++) {
        u32 gamemode_x = x + gamemode_tab * 2;
        u32 gamemode_y = y;
        
        // If the current gamemode is this gamemode, make it blue
        if (gamemode_tab == gamemode) {
            set_tab_palette(5, gamemode_x, gamemode_y);
        } else {
            set_tab_palette(4, gamemode_x, gamemode_y);
        }
    }
}

void set_tab_palette(u32 pal, u32 x, u32 y) {
    u16 *tile = &se_mem[26][x + (y * 32)];
    *tile = SE_BUILD(*tile & SE_ID_MASK, pal, (*tile & SE_HFLIP) >> SE_FLIP_SHIFT, 0);

    tile++;

    *tile = SE_BUILD(*tile & SE_ID_MASK, pal, (*tile & SE_HFLIP) >> SE_FLIP_SHIFT, 0);

    tile += 31;
    
    *tile = SE_BUILD(*tile & SE_ID_MASK, pal, (*tile & SE_HFLIP) >> SE_FLIP_SHIFT, 0);

    tile++;

    *tile = SE_BUILD(*tile & SE_ID_MASK, pal, (*tile & SE_HFLIP) >> SE_FLIP_SHIFT, 0);
}