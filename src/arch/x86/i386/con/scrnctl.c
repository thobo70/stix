/**
 * @file scrnctl.c
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief VGA text mode screen controller for STIX OS
 * @version 0.1
 * @date 2023-11-01
 * 
 * @copyright Copyright (c) 2023
 * 
 * This file implements a VGA text mode screen controller that provides:
 * - Direct VGA memory access (0xB8000)
 * - 80x25 character display
 * - 16 color foreground/background support
 * - Cursor positioning and control
 * - Screen scrolling and clearing
 * - Character attribute management
 */

#include "../../../../include/tdefs.h"
#include "include/scrnctl.h"
#include "include/io.h"

// VGA text mode constants
#define VGA_WIDTH           80      ///< Screen width in characters
#define VGA_HEIGHT          25      ///< Screen height in characters
#define VGA_MEMORY          0xB8000 ///< VGA text mode memory base
#define VGA_CRTC_INDEX      0x3D4   ///< VGA CRTC index port
#define VGA_CRTC_DATA       0x3D5   ///< VGA CRTC data port

// VGA CRTC register indices
#define VGA_CURSOR_HIGH     0x0E    ///< Cursor location high byte
#define VGA_CURSOR_LOW      0x0F    ///< Cursor location low byte

// Default color scheme
#define DEFAULT_COLOR       (VGA_COLOR_LIGHT_GREY | (VGA_COLOR_BLACK << 4))

// Global screen state
static vga_char_t *vga_buffer = (vga_char_t*)VGA_MEMORY;
static int cursor_x = 0;
static int cursor_y = 0;
static byte_t current_color = DEFAULT_COLOR;

/**
 * @brief Read byte from I/O port
 */
static inline byte_t inb(word_t port) {
    byte_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "dN"(port));
    return value;
}

/**
 * @brief Write byte to I/O port
 */
static inline void outb(word_t port, byte_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "dN"(port));
}

/**
 * @brief Update hardware cursor position
 */
static void update_cursor(void) {
    word_t pos = cursor_y * VGA_WIDTH + cursor_x;
    
    outb(VGA_CRTC_INDEX, VGA_CURSOR_HIGH);
    outb(VGA_CRTC_DATA, (pos >> 8) & 0xFF);
    
    outb(VGA_CRTC_INDEX, VGA_CURSOR_LOW);
    outb(VGA_CRTC_DATA, pos & 0xFF);
}

/**
 * @brief Scroll screen up by one line
 */
static void scroll_screen(void) {
    // Move all lines up by one
    for (int y = 0; y < VGA_HEIGHT - 1; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    
    // Clear the last line
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x].character = ' ';
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x].color = current_color;
    }
}

/**
 * @brief Initialize VGA screen controller
 */
void vga_init(void) {
    cursor_x = 0;
    cursor_y = 0;
    current_color = DEFAULT_COLOR;
    
    // Clear screen
    vga_clear_screen();
    update_cursor();
}

/**
 * @brief Clear entire screen
 */
void vga_clear_screen(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i].character = ' ';
        vga_buffer[i].color = current_color;
    }
    cursor_x = 0;
    cursor_y = 0;
    update_cursor();
}

/**
 * @brief Set current text color
 */
void vga_set_color(byte_t color) {
    current_color = color;
}

/**
 * @brief Get current text color
 */
byte_t vga_get_color(void) {
    return current_color;
}

/**
 * @brief Set cursor position
 */
void vga_set_cursor(int x, int y) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        cursor_x = x;
        cursor_y = y;
        update_cursor();
    }
}

/**
 * @brief Get cursor position
 */
void vga_get_cursor(int *x, int *y) {
    if (x) *x = cursor_x;
    if (y) *y = cursor_y;
}

/**
 * @brief Put character at specific position
 */
void vga_put_char_at(char c, int x, int y, byte_t color) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        int index = y * VGA_WIDTH + x;
        vga_buffer[index].character = c;
        vga_buffer[index].color = color;
    }
}

/**
 * @brief Put character at current cursor position
 */
void vga_put_char(char c) {
    switch (c) {
        case '\n':
            cursor_x = 0;
            cursor_y++;
            break;
            
        case '\r':
            cursor_x = 0;
            break;
            
        case '\t':
            cursor_x = (cursor_x + 8) & ~7; // Align to 8-character boundary
            if (cursor_x >= VGA_WIDTH) {
                cursor_x = 0;
                cursor_y++;
            }
            break;
            
        case '\b':
            if (cursor_x > 0) {
                cursor_x--;
                vga_put_char_at(' ', cursor_x, cursor_y, current_color);
            }
            break;
            
        default:
            if (c >= 32 && c <= 126) { // Printable ASCII
                vga_put_char_at(c, cursor_x, cursor_y, current_color);
                cursor_x++;
                
                if (cursor_x >= VGA_WIDTH) {
                    cursor_x = 0;
                    cursor_y++;
                }
            }
            break;
    }
    
    // Handle screen scrolling
    if (cursor_y >= VGA_HEIGHT) {
        scroll_screen();
        cursor_y = VGA_HEIGHT - 1;
    }
    
    update_cursor();
}

/**
 * @brief Write string to screen
 */
void vga_write_string(const char *str) {
    if (!str) return;
    
    while (*str) {
        vga_put_char(*str);
        str++;
    }
}

/**
 * @brief Write string with specific color
 */
void vga_write_string_color(const char *str, byte_t color) {
    byte_t old_color = current_color;
    vga_set_color(color);
    vga_write_string(str);
    vga_set_color(old_color);
}

/**
 * @brief Clear line at specified row
 */
void vga_clear_line(int line) {
    if (line >= 0 && line < VGA_HEIGHT) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[line * VGA_WIDTH + x].character = ' ';
            vga_buffer[line * VGA_WIDTH + x].color = current_color;
        }
    }
}

/**
 * @brief Clear from cursor to end of line
 */
void vga_clear_to_eol(void) {
    for (int x = cursor_x; x < VGA_WIDTH; x++) {
        vga_put_char_at(' ', x, cursor_y, current_color);
    }
}

/**
 * @brief Clear from cursor to end of screen
 */
void vga_clear_to_eos(void) {
    // Clear current line from cursor
    vga_clear_to_eol();
    
    // Clear all lines below
    for (int y = cursor_y + 1; y < VGA_HEIGHT; y++) {
        vga_clear_line(y);
    }
}

/**
 * @brief Insert line at cursor position
 */
void vga_insert_line(void) {
    // Move lines down from cursor position
    for (int y = VGA_HEIGHT - 1; y > cursor_y; y--) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y - 1) * VGA_WIDTH + x];
        }
    }
    
    // Clear the current line
    vga_clear_line(cursor_y);
}

/**
 * @brief Delete line at cursor position
 */
void vga_delete_line(void) {
    // Move lines up from cursor position
    for (int y = cursor_y; y < VGA_HEIGHT - 1; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    
    // Clear the last line
    vga_clear_line(VGA_HEIGHT - 1);
}

/**
 * @brief Enable/disable cursor visibility
 */
void vga_set_cursor_visible(int visible) {
    if (visible) {
        outb(VGA_CRTC_INDEX, 0x0A);
        outb(VGA_CRTC_DATA, inb(VGA_CRTC_DATA) & 0xDF); // Clear bit 5
    } else {
        outb(VGA_CRTC_INDEX, 0x0A);
        outb(VGA_CRTC_DATA, inb(VGA_CRTC_DATA) | 0x20);  // Set bit 5
    }
}
