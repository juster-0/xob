/* xob - A lightweight overlay volume/anything bar for the X Window System.
 * Copyright (C) 2021 Florent Ch.
 *
 * xob is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * xob is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with xob.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include "conf.h"
#include "parser.h"
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xdbe.h>
#include <X11/extensions/Xrandr.h>
#include <stdbool.h>

#define STATE_ALT (0x1)
#define STATE_OVERFLOW (0x1 << 1)
#define STATE_MAPPED (0x1 << 2)

typedef enum
{
    POSITION_RELATIVE_FOCUS,
    POSITION_RELATIVE_POINTER,
    POSITION_COMBINED,
    POSITION_SPECIFIED
} Bar_position;

typedef enum
{
    NORMAL,
    ALTERNATIVE
} Show_mode;

typedef struct
{
    char name[10];
    int x;
    int y;
    int width;
    int height;
} MonitorInfo;

typedef struct
{
    XftColor font_color;
    XftFont *font;
    char *string;
    bool is_dynamic;
    Dynamic_string *pdyn_str;
    struct
    {
        int x;
        int y;
    } pos;
    int width;
    int height;
    Dim x;
    Dim y;
    Align_pos align;
} Text_context;

typedef struct
{
    Text_context *ptext;
    int text_count;
    bool have_dynamic_strings;

    XftDraw *xft_draw;
    Colormap colormap;
    Visual *visual;
} Text_rendering_context;

typedef struct
{
    Display *display;
    int screen_number;
    Screen *screen;
    Window window;
    Bool mapped;
    MonitorInfo monitor_info;
    XdbeBackBuffer back_buffer;
} X_context;

typedef struct
{
    int outline;
    int border;
    int padding;
    int length;
    struct
    {
        double rel;
        int abs;
    } length_dynamic;
    int thickness;
    struct
    {
        double rel;
        int abs;
        int offset;
        int max;
    } x;
    struct
    {
        double rel;
        int abs;
        int offset;
        int max;
    } y;
    Bar_position bar_position;
    Orientation orientation;
    int size_x;
    int size_y;
    int fat_layer;
} Geometry_context;

typedef struct
{
    X_context x;
    Colorscheme colorscheme;
    Geometry_context geometry;
    Text_rendering_context text_rendering;
} Display_context;

Display_context init(Style conf);
void show(Display_context *pdc, int value, int cap, Overflow_mode overflow_mode,
          Show_mode show_mode, char **words_list);
void hide(Display_context *pdc);
void display_context_destroy(Display_context *pdc);

/* Draw a rectangle with the given size, position and color */
void fill_rectangle(X_context xc, Color c, int x, int y, unsigned int w,
                    unsigned int h);

Depth get_display_context_depth(Display_context dc);

#endif /* __DISPLAY_H__ */
