/**
 * @file vt52emu.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief VT52 terminal emulator for STIX OS
 * @version 0.1
 * @date 2023-11-01
 * 
 * @copyright Copyright (c) 2023
 * 
 * This file implements a VT52 terminal emulator that provides:
 * - VT52 escape sequence processing
 * - Cursor positioning and movement
 * - Screen clearing and line editing
 * - Basic terminal control functions
 * - ANSI-style color support (extension)
 */

#include "../../../../include/tdefs.h"
#include "include/scrnctl.h"
#include "include/vt52emu.h"

// VT52 emulator state
typedef struct vt52_state_t {
    int escape_mode;        ///< Current escape sequence mode
    int param_count;        ///< Number of parameters collected
    int params[VT52_MAX_PARAMS]; ///< Parameter values
    int saved_x, saved_y;   ///< Saved cursor position
    byte_t saved_color;     ///< Saved color attribute
} vt52_state_t;

// Global VT52 state
static vt52_state_t vt52_state = {0};

/**
 * @brief Reset VT52 emulator state
 */
static void vt52_reset_state(void) {
    vt52_state.escape_mode = VT52_MODE_NORMAL;
    vt52_state.param_count = 0;
    for (int i = 0; i < VT52_MAX_PARAMS; i++) {
        vt52_state.params[i] = 0;
    }
}

/**
 * @brief Process VT52 escape sequence for cursor positioning
 */
static void vt52_process_cursor_position(char c) {
    if (vt52_state.param_count == 0) {
        // First parameter (row)
        vt52_state.params[0] = c - 32; // VT52 uses space (32) as offset
        vt52_state.param_count++;
    } else if (vt52_state.param_count == 1) {
        // Second parameter (column)
        vt52_state.params[1] = c - 32;
        
        // Set cursor position (VT52 is 1-based, convert to 0-based)
        int row = vt52_state.params[0] - 1;
        int col = vt52_state.params[1] - 1;
        
        if (row >= 0 && row < VGA_HEIGHT && col >= 0 && col < VGA_WIDTH) {
            vga_set_cursor(col, row);
        }
        
        vt52_reset_state();
    }
}

/**
 * @brief Process ANSI-style escape sequences (extension)
 */
static void vt52_process_ansi_sequence(char c) {
    switch (c) {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            // Collect numeric parameter
            if (vt52_state.param_count < VT52_MAX_PARAMS) {
                vt52_state.params[vt52_state.param_count] = 
                    vt52_state.params[vt52_state.param_count] * 10 + (c - '0');
            }
            break;
            
        case ';':
            // Parameter separator
            if (vt52_state.param_count < VT52_MAX_PARAMS - 1) {
                vt52_state.param_count++;
                vt52_state.params[vt52_state.param_count] = 0;
            }
            break;
            
        case 'H': case 'f':
            // Cursor position (row;col)
            {
                int row = (vt52_state.params[0] > 0) ? vt52_state.params[0] - 1 : 0;
                int col = (vt52_state.param_count > 0 && vt52_state.params[1] > 0) ? 
                         vt52_state.params[1] - 1 : 0;
                
                if (row >= VGA_HEIGHT) row = VGA_HEIGHT - 1;
                if (col >= VGA_WIDTH) col = VGA_WIDTH - 1;
                
                vga_set_cursor(col, row);
            }
            vt52_reset_state();
            break;
            
        case 'A':
            // Cursor up
            {
                int x, y;
                vga_get_cursor(&x, &y);
                int move = (vt52_state.params[0] > 0) ? vt52_state.params[0] : 1;
                y = (y - move < 0) ? 0 : y - move;
                vga_set_cursor(x, y);
            }
            vt52_reset_state();
            break;
            
        case 'B':
            // Cursor down
            {
                int x, y;
                vga_get_cursor(&x, &y);
                int move = (vt52_state.params[0] > 0) ? vt52_state.params[0] : 1;
                y = (y + move >= VGA_HEIGHT) ? VGA_HEIGHT - 1 : y + move;
                vga_set_cursor(x, y);
            }
            vt52_reset_state();
            break;
            
        case 'C':
            // Cursor right
            {
                int x, y;
                vga_get_cursor(&x, &y);
                int move = (vt52_state.params[0] > 0) ? vt52_state.params[0] : 1;
                x = (x + move >= VGA_WIDTH) ? VGA_WIDTH - 1 : x + move;
                vga_set_cursor(x, y);
            }
            vt52_reset_state();
            break;
            
        case 'D':
            // Cursor left
            {
                int x, y;
                vga_get_cursor(&x, &y);
                int move = (vt52_state.params[0] > 0) ? vt52_state.params[0] : 1;
                x = (x - move < 0) ? 0 : x - move;
                vga_set_cursor(x, y);
            }
            vt52_reset_state();
            break;
            
        case 'J':
            // Clear screen
            if (vt52_state.params[0] == 0) {
                vga_clear_to_eos();
            } else if (vt52_state.params[0] == 2) {
                vga_clear_screen();
            }
            vt52_reset_state();
            break;
            
        case 'K':
            // Clear line
            if (vt52_state.params[0] == 0) {
                vga_clear_to_eol();
            } else if (vt52_state.params[0] == 2) {
                int x, y;
                vga_get_cursor(&x, &y);
                vga_clear_line(y);
                vga_set_cursor(0, y);
            }
            vt52_reset_state();
            break;
            
        case 'm':
            // Set graphics mode (color)
            {
                byte_t fg = VGA_COLOR_LIGHT_GREY;
                byte_t bg = VGA_COLOR_BLACK;
                
                for (int i = 0; i <= vt52_state.param_count; i++) {
                    switch (vt52_state.params[i]) {
                        case 0: // Reset
                            fg = VGA_COLOR_LIGHT_GREY;
                            bg = VGA_COLOR_BLACK;
                            break;
                        case 30: fg = VGA_COLOR_BLACK; break;
                        case 31: fg = VGA_COLOR_RED; break;
                        case 32: fg = VGA_COLOR_GREEN; break;
                        case 33: fg = VGA_COLOR_BROWN; break;
                        case 34: fg = VGA_COLOR_BLUE; break;
                        case 35: fg = VGA_COLOR_MAGENTA; break;
                        case 36: fg = VGA_COLOR_CYAN; break;
                        case 37: fg = VGA_COLOR_LIGHT_GREY; break;
                        case 40: bg = VGA_COLOR_BLACK; break;
                        case 41: bg = VGA_COLOR_RED; break;
                        case 42: bg = VGA_COLOR_GREEN; break;
                        case 43: bg = VGA_COLOR_BROWN; break;
                        case 44: bg = VGA_COLOR_BLUE; break;
                        case 45: bg = VGA_COLOR_MAGENTA; break;
                        case 46: bg = VGA_COLOR_CYAN; break;
                        case 47: bg = VGA_COLOR_LIGHT_GREY; break;
                    }
                }
                
                vga_set_color(vga_make_color(fg, bg));
            }
            vt52_reset_state();
            break;
            
        default:
            // Unknown sequence, reset
            vt52_reset_state();
            break;
    }
}

/**
 * @brief Initialize VT52 terminal emulator
 */
void vt52_init(void) {
    vt52_reset_state();
    vga_init();
}

/**
 * @brief Process a character through VT52 emulator
 */
void vt52_process_char(char c) {
    switch (vt52_state.escape_mode) {
        case VT52_MODE_NORMAL:
            if (c == 27) { // ESC
                vt52_state.escape_mode = VT52_MODE_ESCAPE;
            } else {
                vga_put_char(c);
            }
            break;
            
        case VT52_MODE_ESCAPE:
            switch (c) {
                case 'A':
                    // Cursor up
                    {
                        int x, y;
                        vga_get_cursor(&x, &y);
                        if (y > 0) vga_set_cursor(x, y - 1);
                    }
                    vt52_reset_state();
                    break;
                    
                case 'B':
                    // Cursor down
                    {
                        int x, y;
                        vga_get_cursor(&x, &y);
                        if (y < VGA_HEIGHT - 1) vga_set_cursor(x, y + 1);
                    }
                    vt52_reset_state();
                    break;
                    
                case 'C':
                    // Cursor right
                    {
                        int x, y;
                        vga_get_cursor(&x, &y);
                        if (x < VGA_WIDTH - 1) vga_set_cursor(x + 1, y);
                    }
                    vt52_reset_state();
                    break;
                    
                case 'D':
                    // Cursor left
                    {
                        int x, y;
                        vga_get_cursor(&x, &y);
                        if (x > 0) vga_set_cursor(x - 1, y);
                    }
                    vt52_reset_state();
                    break;
                    
                case 'H':
                    // Cursor home
                    vga_set_cursor(0, 0);
                    vt52_reset_state();
                    break;
                    
                case 'I':
                    // Reverse line feed
                    {
                        int x, y;
                        vga_get_cursor(&x, &y);
                        if (y > 0) {
                            vga_set_cursor(x, y - 1);
                        } else {
                            vga_insert_line();
                        }
                    }
                    vt52_reset_state();
                    break;
                    
                case 'J':
                    // Clear to end of screen
                    vga_clear_to_eos();
                    vt52_reset_state();
                    break;
                    
                case 'K':
                    // Clear to end of line
                    vga_clear_to_eol();
                    vt52_reset_state();
                    break;
                    
                case 'L':
                    // Insert line
                    vga_insert_line();
                    vt52_reset_state();
                    break;
                    
                case 'M':
                    // Delete line
                    vga_delete_line();
                    vt52_reset_state();
                    break;
                    
                case 'Y':
                    // Direct cursor addressing
                    vt52_state.escape_mode = VT52_MODE_CURSOR_POS;
                    vt52_state.param_count = 0;
                    break;
                    
                case 'Z':
                    // Identify terminal (respond with ESC/K)
                    vt52_write_string("\033/K");
                    vt52_reset_state();
                    break;
                    
                case '[':
                    // ANSI escape sequence (extension)
                    vt52_state.escape_mode = VT52_MODE_ANSI;
                    vt52_state.param_count = 0;
                    vt52_state.params[0] = 0;
                    break;
                    
                case '7':
                    // Save cursor position
                    vga_get_cursor(&vt52_state.saved_x, &vt52_state.saved_y);
                    vt52_state.saved_color = vga_get_color();
                    vt52_reset_state();
                    break;
                    
                case '8':
                    // Restore cursor position
                    vga_set_cursor(vt52_state.saved_x, vt52_state.saved_y);
                    vga_set_color(vt52_state.saved_color);
                    vt52_reset_state();
                    break;
                    
                default:
                    // Unknown escape sequence
                    vt52_reset_state();
                    break;
            }
            break;
            
        case VT52_MODE_CURSOR_POS:
            vt52_process_cursor_position(c);
            break;
            
        case VT52_MODE_ANSI:
            vt52_process_ansi_sequence(c);
            break;
            
        default:
            vt52_reset_state();
            break;
    }
}

/**
 * @brief Write string through VT52 emulator
 */
void vt52_write_string(const char *str) {
    if (!str) return;
    
    while (*str) {
        vt52_process_char(*str);
        str++;
    }
}

/**
 * @brief Clear screen via VT52 command
 */
void vt52_clear_screen(void) {
    vt52_write_string("\033H\033J");
}

/**
 * @brief Set cursor position via VT52 command
 */
void vt52_set_cursor(int x, int y) {
    char cmd[8];
    cmd[0] = '\033';
    cmd[1] = 'Y';
    cmd[2] = (char)(y + 32 + 1); // VT52 is 1-based, add space offset
    cmd[3] = (char)(x + 32 + 1);
    cmd[4] = '\0';
    
    vt52_write_string(cmd);
}

/**
 * @brief Reset VT52 terminal
 */
void vt52_reset(void) {
    vt52_reset_state();
    vga_clear_screen();
    vga_set_cursor(0, 0);
    vga_set_color(vga_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
}