/**
 * @file scrnctl.h
 * @author Thomas Boos (tboos70@gmail.com)
 * @brief VGA text mode screen controller interface
 * @version 0.1
 * @date 2023-11-01
 * 
 * @copyright Copyright (c) 2023
 * 
 * This header defines the interface for VGA text mode screen control,
 * providing functions for character output, cursor control, and screen
 * manipulation.
 */

#ifndef _SCRNCTL_H
#define _SCRNCTL_H

#include "../../../../include/tdefs.h"

// VGA text mode constants
#define VGA_WIDTH           80      ///< Screen width in characters
#define VGA_HEIGHT          25      ///< Screen height in characters

// VGA color constants
#define VGA_COLOR_BLACK         0   ///< Black color
#define VGA_COLOR_BLUE          1   ///< Blue color
#define VGA_COLOR_GREEN         2   ///< Green color
#define VGA_COLOR_CYAN          3   ///< Cyan color
#define VGA_COLOR_RED           4   ///< Red color
#define VGA_COLOR_MAGENTA       5   ///< Magenta color
#define VGA_COLOR_BROWN         6   ///< Brown color
#define VGA_COLOR_LIGHT_GREY    7   ///< Light grey color
#define VGA_COLOR_DARK_GREY     8   ///< Dark grey color
#define VGA_COLOR_LIGHT_BLUE    9   ///< Light blue color
#define VGA_COLOR_LIGHT_GREEN   10  ///< Light green color
#define VGA_COLOR_LIGHT_CYAN    11  ///< Light cyan color
#define VGA_COLOR_LIGHT_RED     12  ///< Light red color
#define VGA_COLOR_LIGHT_MAGENTA 13  ///< Light magenta color
#define VGA_COLOR_LIGHT_BROWN   14  ///< Light brown color
#define VGA_COLOR_WHITE         15  ///< White color

/**
 * @brief VGA character structure
 * 
 * Each character in VGA text mode consists of a character byte
 * and an attribute byte containing color information.
 */
typedef struct vga_char_t {
    byte_t character;   ///< ASCII character
    byte_t color;       ///< Color attribute (foreground | background << 4)
} __attribute__((packed)) vga_char_t;

/**
 * @brief Create VGA color attribute
 * 
 * @param fg Foreground color (0-15)
 * @param bg Background color (0-15)
 * @return byte_t Combined color attribute
 */
static inline byte_t vga_make_color(byte_t fg, byte_t bg) {
    return fg | (bg << 4);
}

// Screen control functions

/**
 * @brief Initialize VGA screen controller
 */
void vga_init(void);

/**
 * @brief Clear entire screen
 */
void vga_clear_screen(void);

/**
 * @brief Set current text color
 * 
 * @param color Color attribute byte
 */
void vga_set_color(byte_t color);

/**
 * @brief Get current text color
 * 
 * @return byte_t Current color attribute
 */
byte_t vga_get_color(void);

// Cursor control functions

/**
 * @brief Set cursor position
 * 
 * @param x Column position (0-79)
 * @param y Row position (0-24)
 */
void vga_set_cursor(int x, int y);

/**
 * @brief Get cursor position
 * 
 * @param x Pointer to store column position
 * @param y Pointer to store row position
 */
void vga_get_cursor(int *x, int *y);

/**
 * @brief Enable/disable cursor visibility
 * 
 * @param visible Non-zero to show cursor, zero to hide
 */
void vga_set_cursor_visible(int visible);

// Character output functions

/**
 * @brief Put character at specific position
 * 
 * @param c Character to display
 * @param x Column position
 * @param y Row position
 * @param color Color attribute
 */
void vga_put_char_at(char c, int x, int y, byte_t color);

/**
 * @brief Put character at current cursor position
 * 
 * @param c Character to display
 */
void vga_put_char(char c);

/**
 * @brief Write string to screen
 * 
 * @param str Null-terminated string
 */
void vga_write_string(const char *str);

/**
 * @brief Write string with specific color
 * 
 * @param str Null-terminated string
 * @param color Color attribute
 */
void vga_write_string_color(const char *str, byte_t color);

// Screen manipulation functions

/**
 * @brief Clear line at specified row
 * 
 * @param line Row number (0-24)
 */
void vga_clear_line(int line);

/**
 * @brief Clear from cursor to end of line
 */
void vga_clear_to_eol(void);

/**
 * @brief Clear from cursor to end of screen
 */
void vga_clear_to_eos(void);

/**
 * @brief Insert line at cursor position
 */
void vga_insert_line(void);

/**
 * @brief Delete line at cursor position
 */
void vga_delete_line(void);

#endif /* _SCRNCTL_H */
