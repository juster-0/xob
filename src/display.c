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

#include "display.h"

#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Keep value in range */
static int fit_in(int value, int min, int max)
{
    value = (value < min) ? min : value;
    value = (value > max) ? max : value;
    return value;
}

/* Horizontal and vertical size depending on orientation */
static int size_x(Geometry_context g)
{
    return g.orientation == HORIZONTAL ? g.length : g.thickness;
}

static int size_y(Geometry_context g)
{
    return g.orientation == HORIZONTAL ? g.thickness : g.length;
}

/* Draw an empty bar with the given colors */
static void draw_empty(X_context x, Geometry_context g, Colors colors)
{
    /* Fill with transparent layer so other windows can update
     * content behind the bar (works only with compositors) */
    Color transparent = {.red = 0x0, .green = 0x0, .blue = 0x0, .alpha = 0x0};
    fill_rectangle(x, transparent, 0, 0, g.size_x + g.x.offset,
                   g.size_y + g.y.offset);

    /* Outline */
    /* Left */
    fill_rectangle(x, colors.bg, 0 + g.x.offset, 0 + g.y.offset, g.outline,
                   2 * (g.outline + g.border + g.padding) + size_y(g));

    /* Right */
    fill_rectangle(x, colors.bg,
                   2 * (g.border + g.padding) + g.outline + size_x(g) +
                       g.x.offset,
                   0 + g.y.offset, g.outline,
                   2 * (g.outline + g.border + g.padding) + size_y(g));

    /* Top */
    fill_rectangle(x, colors.bg, 0 + g.x.offset, 0 + g.y.offset,
                   2 * (g.outline + g.border + g.padding) + size_x(g),
                   g.outline);

    /* Bottom */
    fill_rectangle(
        x, colors.bg, 0 + g.x.offset,
        2 * (g.border + g.padding) + g.outline + size_y(g) + g.y.offset,
        2 * (g.outline + g.border + g.padding) + size_x(g), g.outline);

    /* Border */
    /* Left */
    fill_rectangle(x, colors.border, g.outline + g.x.offset,
                   g.outline + g.y.offset, g.border,
                   2 * (g.border + g.padding) + size_y(g));

    /* Right */
    fill_rectangle(x, colors.border,
                   g.outline + g.border + 2 * g.padding + size_x(g) +
                       g.x.offset,
                   g.outline + g.y.offset, g.border,
                   2 * (g.border + g.padding) + size_y(g));

    /* Top */
    fill_rectangle(x, colors.border, g.outline + g.x.offset,
                   g.outline + g.y.offset,
                   2 * (g.border + g.padding) + size_x(g), g.border);

    /* Bottom */
    fill_rectangle(x, colors.border, g.outline + g.x.offset,
                   g.outline + g.border + 2 * g.padding + size_y(g) +
                       g.y.offset,
                   2 * (g.border + g.padding) + size_x(g), g.border);

    /* Padding */
    /* Left */
    fill_rectangle(x, colors.bg, g.outline + g.border + g.x.offset,
                   g.outline + g.border + g.y.offset, g.padding,
                   2 * g.padding + size_y(g));

    /* Right */
    fill_rectangle(x, colors.bg,
                   g.outline + g.border + g.padding + size_x(g) + g.x.offset,
                   g.outline + g.border + g.y.offset, g.padding,
                   2 * g.padding + size_y(g));

    /* Top */
    fill_rectangle(x, colors.bg, g.outline + g.border + g.x.offset,
                   g.outline + g.border + g.y.offset, 2 * g.padding + size_x(g),
                   g.padding);

    /* Bottom */
    fill_rectangle(x, colors.bg, g.outline + g.border + g.x.offset,
                   g.outline + g.border + g.padding + size_y(g) + g.y.offset,
                   2 * g.padding + size_x(g), g.padding);
}

/* Draw a given length of filled bar with the given color */
static void draw_content(X_context x, Geometry_context g, int filled_length,
                         Colors colors)
{
    if (g.orientation == HORIZONTAL)
    {
        /* Fill foreground color */
        fill_rectangle(x, colors.fg,
                       g.outline + g.border + g.padding + g.x.offset,
                       g.outline + g.border + g.padding + g.y.offset,
                       filled_length, g.thickness);

        /* Fill background color */
        fill_rectangle(x, colors.bg,
                       g.outline + g.border + g.padding + filled_length +
                           g.x.offset,
                       g.outline + g.border + g.padding + g.y.offset,
                       g.length - filled_length, g.thickness);
    }
    else
    {
        /* fill foreground color */
        fill_rectangle(x, colors.fg,
                       g.outline + g.border + g.padding + g.x.offset,
                       g.outline + g.border + g.padding + g.length -
                           filled_length + g.y.offset,
                       g.thickness, filled_length);

        /* Fill background color */
        fill_rectangle(x, colors.bg,
                       g.outline + g.border + g.padding + g.x.offset,
                       g.outline + g.border + g.padding + g.y.offset,
                       g.thickness, g.length - filled_length);
    }
}

/* Draw a separator (padding-sized gap) at the given position */
static void draw_separator(X_context x, Geometry_context g, int position,
                           Color color)
{
    if (g.orientation == HORIZONTAL)
    {
        fill_rectangle(x, color,
                       g.outline + g.border + (g.padding / 2) + position +
                           g.x.offset,
                       g.outline + g.border + g.padding + g.y.offset, g.padding,
                       g.thickness);
    }
    else
    {
        fill_rectangle(x, color, g.outline + g.border + g.padding + g.x.offset,
                       g.outline + g.border + (g.padding / 2) + g.length -
                           position + g.y.offset,
                       g.thickness, g.padding);
    }
}

void compute_geometry(Display_context *pdc, int *topleft_x, int *topleft_y)
{
    /* Orientation-related dimensions */
    int available_length = pdc->geometry.orientation == HORIZONTAL
                               ? pdc->x.monitor_info.width
                               : pdc->x.monitor_info.height;

    /* Calculate bar sizes */
    pdc->geometry.length =
        fit_in(available_length * pdc->geometry.length_dynamic.rel +
                   pdc->geometry.length_dynamic.abs,
               0, available_length - 2 * pdc->geometry.fat_layer);

    pdc->geometry.size_x = size_x(pdc->geometry) + 2 * pdc->geometry.fat_layer;
    pdc->geometry.size_y = size_y(pdc->geometry) + 2 * pdc->geometry.fat_layer;

    /* Compute position of the top-left corner */
    *topleft_x = fit_in(pdc->x.monitor_info.width * pdc->geometry.x.rel -
                            pdc->geometry.size_x / 2,
                        0, pdc->x.monitor_info.width - pdc->geometry.size_x) +
                 pdc->geometry.x.abs + pdc->x.monitor_info.x;
    *topleft_y = fit_in(pdc->x.monitor_info.height * pdc->geometry.y.rel -
                            pdc->geometry.size_y / 2,
                        0, pdc->x.monitor_info.height - pdc->geometry.size_y) +
                 pdc->geometry.y.abs + pdc->x.monitor_info.y;
}

static void compute_text_position(Display_context *pdc)
{
    /* Calculate text postion relative bar */
    pdc->text_rendering.ptext->pos.x =
        pdc->text_rendering.ptext->x.rel * pdc->geometry.size_x -
        pdc->text_rendering.ptext->width * pdc->text_rendering.ptext->align.x +
        pdc->text_rendering.ptext->x.abs;
    pdc->text_rendering.ptext->pos.y =
        pdc->text_rendering.ptext->y.rel * pdc->geometry.size_y +
        pdc->text_rendering.ptext->height *
            (1.0 - pdc->text_rendering.ptext->align.y) +
        pdc->text_rendering.ptext->y.abs;

    /* Calculate offset x*/
    if (pdc->text_rendering.ptext->pos.x < 0)
    {
        pdc->geometry.x.offset = -pdc->text_rendering.ptext->pos.x;
        pdc->text_rendering.ptext->pos.x = 0;
    }
    else
        pdc->geometry.x.offset = 0;

    /* Calculate offset y */
    if (pdc->text_rendering.ptext->pos.y - pdc->text_rendering.ptext->height <
        0)
    {
        pdc->geometry.y.offset = -(pdc->text_rendering.ptext->pos.y -
                                   pdc->text_rendering.ptext->height);
        pdc->text_rendering.ptext->pos.y = pdc->text_rendering.ptext->height;
    }
    else
        pdc->geometry.y.offset = 0;

    /* Calculate window width */
    if (pdc->geometry.x.offset + pdc->geometry.size_x >
        pdc->text_rendering.ptext->width + pdc->text_rendering.ptext->pos.x)
        pdc->geometry.size_x = pdc->geometry.x.offset + pdc->geometry.size_x;
    else
        pdc->geometry.size_x =
            pdc->text_rendering.ptext->width + pdc->text_rendering.ptext->pos.x;

    /* Calculate window height */
    if (pdc->text_rendering.ptext->pos.y >
        pdc->geometry.y.offset + pdc->geometry.size_y)
        pdc->geometry.size_y = pdc->text_rendering.ptext->pos.y;
    else
        pdc->geometry.size_y = pdc->geometry.y.offset + pdc->geometry.size_y;
}

/* Set combined positon */
static void set_combined_position(Display_context *pdc)
{
    pdc->x.monitor_info.x = 0;
    pdc->x.monitor_info.y = 0;
    pdc->x.monitor_info.width = WidthOfScreen(pdc->x.screen);
    pdc->x.monitor_info.height = HeightOfScreen(pdc->x.screen);
    // strcpy(pdc->x.monitor_info.name, MONITOR_COMBINED);
}

/* Set specified monitor */
static void set_specified_position(Display_context *pdc, const Style *pconf)
{
    Window root = RootWindow(pdc->x.display, pdc->x.screen_number);

    /* Get monitors info */
    int num_monitors;
    char *monitor_name;
    XRRMonitorInfo *monitor_sizes =
        XRRGetMonitors(pdc->x.display, root, 0, &num_monitors);

    /* Compare monitors output names */
    int i;
    for (i = 0; i < num_monitors; i++)
    {
        monitor_name = XGetAtomName(pdc->x.display, monitor_sizes[i].name);
        if (strcmp(pconf->monitor, monitor_name) == 0)
            break;
    }
    if (i != num_monitors)
    {
        pdc->x.monitor_info.x = monitor_sizes[i].x;
        pdc->x.monitor_info.y = monitor_sizes[i].y;
        pdc->x.monitor_info.width = monitor_sizes[i].width;
        pdc->x.monitor_info.height = monitor_sizes[i].height;
        strcpy(pdc->x.monitor_info.name, monitor_name);
    }
    else // Monitor name is not found
    {
        /* Use combined surface for monitor option if no monitors with
         * provided name found */
        fprintf(stderr, "Error: monitor %s is not found.\n", pconf->monitor);
        fprintf(stderr, "Info: falling back to combined mode.\n");
        set_combined_position(pdc);
        // strcpy(pdc.x.monitor_info.name, MONITOR_COMBINED);
    }
    XRRFreeMonitors(monitor_sizes);
}

/* Move and resize the bar relative to a monitor with provided coords */
static void move_resize_to_coords_monitor(Display_context *pdc, int x, int y)
{
    int i;
    int topleft_x, topleft_y;
    int num_monitors;
    XRRMonitorInfo *monitor_sizes;

    monitor_sizes = XRRGetMonitors(
        pdc->x.display, RootWindow(pdc->x.display, pdc->x.screen_number), 0,
        &num_monitors);
    for (i = 0; i < num_monitors; i++)
    {
        /* Find monitor by coords */
        if (x >= monitor_sizes[i].x &&
            x < monitor_sizes[i].x + monitor_sizes[i].width &&
            y >= monitor_sizes[i].y &&
            y < monitor_sizes[i].y + monitor_sizes[i].height)
        {
            break;
        }
    }
    pdc->x.monitor_info.width = monitor_sizes[i].width;
    pdc->x.monitor_info.height = monitor_sizes[i].height;
    pdc->x.monitor_info.x = monitor_sizes[i].x;
    pdc->x.monitor_info.y = monitor_sizes[i].y;
    XRRFreeMonitors(monitor_sizes);

    compute_geometry(pdc, &topleft_x, &topleft_y);

    if (pdc->text_rendering.text_count != 0)
        compute_text_position(pdc);

    XMoveResizeWindow(pdc->x.display, pdc->x.window,
                      topleft_x - pdc->geometry.x.offset,
                      topleft_y - pdc->geometry.y.offset, pdc->geometry.size_x,
                      pdc->geometry.size_y);
}

/* Mobe the bar to monitor with focused window */
static void move_resize_to_focused_monitor(Display_context *pdc)
{
    int revert_to_window;
    int focused_x, focused_y;
    int dummy_x, dummy_y;
    unsigned int focused_width, focused_height, focused_border, focused_depth;

    Window focused_window, fchild_window;

    XGetInputFocus(pdc->x.display, &focused_window, &revert_to_window);

    /* Get coords of focused window */
    XTranslateCoordinates(pdc->x.display, focused_window,
                          RootWindow(pdc->x.display, pdc->x.screen_number), 0,
                          0, &focused_x, &focused_y, &fchild_window);
    /* Get focused window width and height to move bar relative to
     * the center of focused window */
    XGetGeometry(pdc->x.display, focused_window, &fchild_window, &dummy_x,
                 &dummy_y, &focused_width, &focused_height, &focused_border,
                 &focused_depth);

    move_resize_to_coords_monitor(pdc, focused_x + focused_width / 2,
                                  focused_y + focused_height / 2);
}

/* Move the bar to monitor with pointer */
static void move_resize_to_pointer_monitor(Display_context *pdc)
{
    int pointer_x, pointer_y, win_x, win_y;
    unsigned int p_mask;
    Window p_root, p_child;

    XQueryPointer(pdc->x.display,
                  RootWindow(pdc->x.display, pdc->x.screen_number), &p_root,
                  &p_child, &pointer_x, &pointer_y, &win_x, &win_y, &p_mask);

    move_resize_to_coords_monitor(pdc, pointer_x, pointer_y);
}

static void init_text(Display_context *pdc, const Style *pconf)
{
    int i, str_len;
    Depth dc_depth;
    XGlyphInfo text_info;

    pdc->text_rendering.text_count = pconf->text_list.len;

    /* if no text found in conf then not init text rendering */
    if (pdc->text_rendering.text_count == 0)
    {
        pdc->geometry.x.offset = 0;
        pdc->geometry.y.offset = 0;
        pdc->text_rendering.ptext = NULL;
        return;
    }
    pdc->text_rendering.ptext = (Text_context *)malloc(
        sizeof(Text_context) * pdc->text_rendering.text_count);

    dc_depth = get_display_context_depth(*pdc);
    pdc->text_rendering.colormap = XCreateColormap(
        pdc->x.display, RootWindow(pdc->x.display, pdc->x.screen_number),
        dc_depth.visuals, AllocNone);
    pdc->text_rendering.visual = dc_depth.visuals;

    for (i = 0; i < pdc->text_rendering.text_count; i++)
    {
        /* Copy text position data to Text_context structure */
        pdc->text_rendering.ptext[i].x.rel = pconf->text_list.ptext[i].x.rel;
        pdc->text_rendering.ptext[i].x.abs = pconf->text_list.ptext[i].x.abs;
        pdc->text_rendering.ptext[i].y.rel = pconf->text_list.ptext[i].y.rel;
        pdc->text_rendering.ptext[i].y.abs = pconf->text_list.ptext[i].y.abs;

        pdc->text_rendering.ptext[i].align.x =
            pconf->text_list.ptext[i].align.x;
        pdc->text_rendering.ptext[i].align.y =
            pconf->text_list.ptext[i].align.y;

        /*** Load and configure fonts and colors ***/

        /* Copy string to context */
        str_len = strlen(pconf->text_list.ptext[i].string);
        pdc->text_rendering.ptext[i].string = (char *)malloc(str_len + 1);
        strcpy(pdc->text_rendering.ptext[i].string,
               pconf->text_list.ptext[i].string);
        pdc->text_rendering.ptext[i].string[str_len] = '\0';

        /* Load font */
        pdc->text_rendering.ptext[i].font =
            XftFontOpenName(pdc->x.display, pdc->x.screen_number,
                            pconf->text_list.ptext[i].font_name);
        if (pdc->text_rendering.ptext[i].font)
            fprintf(stderr, "Info: Loaded font \"%s\"\n",
                    pconf->text_list.ptext[i].font_name);
        else
            fprintf(stderr, "Error: Font \"%s\" is not loaded\n",
                    pconf->text_list.ptext[i].font_name);

        /* Load color */
        if (!XftColorAllocName(pdc->x.display, pdc->text_rendering.visual,
                               pdc->text_rendering.colormap,
                               pconf->text_list.ptext[i].color,
                               &pdc->text_rendering.ptext[i].font_color))
            fprintf(stderr, "Error: Color \"%s\" is not loaded\n",
                    pconf->text_list.ptext[i].color);

        /* Calculate text sizes */
        XftTextExtentsUtf8(pdc->x.display, pdc->text_rendering.ptext[i].font,
                           (const FcChar8 *)pdc->text_rendering.ptext[i].string,
                           str_len, &text_info);
        pdc->text_rendering.ptext[i].width = text_info.width;
        // dc.text_rendering.ptext->height = text_info.height;
        pdc->text_rendering.ptext[i].height = text_info.y;
    }
    compute_text_position(pdc);
}

/* PUBLIC Returns a new display context from a given configuration. If the
 * .x.display field of the returned display context is NULL, display could not
 * have been opened.*/
Display_context init(Style conf)
{
    Display_context dc;
    Depth dc_depth;
    Window root;
    int topleft_x;
    int topleft_y;
    XSetWindowAttributes window_attributes;
    static long window_attributes_flags =
        CWColormap | CWBorderPixel | CWOverrideRedirect;

    dc.x.display = XOpenDisplay(NULL);
    if (dc.x.display != NULL)
    {
        dc.x.screen_number = DefaultScreen(dc.x.display);
        dc.x.screen = ScreenOfDisplay(dc.x.display, dc.x.screen_number);
        root = RootWindow(dc.x.display, dc.x.screen_number);

        dc_depth = get_display_context_depth(dc);

        window_attributes.colormap =
            XCreateColormap(dc.x.display, root, dc_depth.visuals, AllocNone);
        window_attributes.border_pixel = 0;
        window_attributes.override_redirect = True;

        /* Get bar position from conf */
        if (strcmp(conf.monitor, MONITOR_RELATIVE_FOCUS) == 0)
            dc.geometry.bar_position = POSITION_RELATIVE_FOCUS;
        else if (strcmp(conf.monitor, MONITOR_RELATIVE_POINTER) == 0)
            dc.geometry.bar_position = POSITION_RELATIVE_POINTER;
        else if (strcmp(conf.monitor, MONITOR_COMBINED) == 0)
            dc.geometry.bar_position = POSITION_COMBINED;
        else
            dc.geometry.bar_position = POSITION_SPECIFIED;

        switch (dc.geometry.bar_position)
        {
        case POSITION_RELATIVE_FOCUS:
        case POSITION_RELATIVE_POINTER:
            /* Bar position and sizes will be recalculated every time before
             * showing, so the code just init position and sizes like for
             * combined surface */
            set_combined_position(&dc);
            break;
        case POSITION_COMBINED:
            set_combined_position(&dc);
            break;
        case POSITION_SPECIFIED:
            set_specified_position(&dc, &conf);
            break;
        default:
            fprintf(stderr, "Error: in switch position\n");
            break;
        }

        /* Write bar position relative data to X_context */
        dc.geometry.x.rel = conf.x.rel;
        dc.geometry.x.abs = conf.x.abs;
        dc.geometry.y.rel = conf.y.rel;
        dc.geometry.y.abs = conf.y.abs;

        dc.geometry.outline = conf.outline;
        dc.geometry.border = conf.border;
        dc.geometry.padding = conf.padding;
        dc.geometry.thickness = conf.thickness;
        dc.geometry.orientation = conf.orientation;
        dc.geometry.length_dynamic.rel = conf.length.rel;
        dc.geometry.length_dynamic.abs = conf.length.abs;

        dc.geometry.fat_layer =
            dc.geometry.padding + dc.geometry.border + dc.geometry.outline;

        compute_geometry(&dc, &topleft_x, &topleft_y);

        /* init text context */
        init_text(&dc, &conf);

        /* Creation of the window */
        dc.x.window = XCreateWindow(
            dc.x.display, root, topleft_x - dc.geometry.x.offset,
            topleft_y - dc.geometry.y.offset, dc.geometry.size_x,
            dc.geometry.size_y, 0, dc_depth.depth, InputOutput,
            dc_depth.visuals, window_attributes_flags, &window_attributes);

        dc.text_rendering.xft_draw =
            XftDrawCreate(dc.x.display, dc.x.window, dc.text_rendering.visual,
                          dc.text_rendering.colormap);

        /* Set a WM_CLASS for the window */
        XClassHint *class_hint = XAllocClassHint();
        if (class_hint != NULL)
        {
            class_hint->res_name = DEFAULT_CONFIG_APPNAME;
            class_hint->res_class = DEFAULT_CONFIG_APPNAME;
            XSetClassHint(dc.x.display, dc.x.window, class_hint);
            XFree(class_hint);
        }

        /* The new window is not mapped yet */
        dc.x.mapped = False;

        /* Colorscheme */
        dc.colorscheme = conf.colorscheme;
    }

    return dc;
}

/* PUBLIC Cleans the X memory buffers. */
void display_context_destroy(Display_context *pdc)
{
    int i;
    for (i = 0; i < pdc->text_rendering.text_count; i++)
    {
        free(pdc->text_rendering.ptext[i].string);
        XftColorFree(pdc->x.display, pdc->text_rendering.visual,
                     pdc->text_rendering.colormap,
                     &pdc->text_rendering.ptext[i].font_color);
    }
    free(pdc->text_rendering.ptext);

    XftDrawDestroy(pdc->text_rendering.xft_draw);

    XCloseDisplay(pdc->x.display);
}

/* PUBLIC Show a bar filled at value/cap in normal or alternative mode */
void show(Display_context *pdc, int value, int cap, Overflow_mode overflow_mode,
          Show_mode show_mode)
{
    Colors colors;
    Colors colors_overflow_proportional;
    static int_fast8_t current_state = 0x0;
    static int_fast8_t last_state = 0x0;

    int old_length = pdc->geometry.length;

    /* Move the bar for relative positions */
    switch (pdc->geometry.bar_position)
    {
    case POSITION_RELATIVE_FOCUS:
        move_resize_to_focused_monitor(pdc);
        break;
    case POSITION_RELATIVE_POINTER:
        move_resize_to_pointer_monitor(pdc);
        break;
    case POSITION_COMBINED:
    case POSITION_SPECIFIED:
        break;
    }

    if (!pdc->x.mapped)
    {
        XMapWindow(pdc->x.display, pdc->x.window);
        XRaiseWindow(pdc->x.display, pdc->x.window);
        pdc->x.mapped = True;
        current_state ^= STATE_MAPPED;
    }

    switch (show_mode)
    {
    case NORMAL:
        colors_overflow_proportional = pdc->colorscheme.normal;
        current_state &= ~STATE_ALT;
        if (value <= cap)
        {
            colors = pdc->colorscheme.normal;
            current_state &= ~STATE_OVERFLOW;
        }
        else
        {
            colors = pdc->colorscheme.overflow;
            colors_overflow_proportional.bg = colors.fg;
            current_state |= STATE_OVERFLOW;
        }
        break;

    case ALTERNATIVE:
        colors_overflow_proportional = pdc->colorscheme.alt;
        current_state |= STATE_ALT;
        if (value <= cap)
        {
            colors = pdc->colorscheme.alt;
            current_state &= ~STATE_OVERFLOW;
        }
        else
        {
            colors = pdc->colorscheme.altoverflow;
            colors_overflow_proportional.bg = colors.fg;
            current_state |= STATE_OVERFLOW;
        }
        break;
    }

    /* Redraw empty bar only if needed */
    if (last_state != current_state || old_length != pdc->geometry.length)
    {
        /* Empty bar */
        draw_empty(pdc->x, pdc->geometry, colors);
    }
    last_state = current_state;

    /* Proportional overflow : draw separator */
    if (value > cap && overflow_mode == PROPORTIONAL &&
        cap * pdc->geometry.length / value > pdc->geometry.padding)
    {
        draw_content(pdc->x, pdc->geometry, cap * pdc->geometry.length / value,
                     colors_overflow_proportional);
        draw_separator(pdc->x, pdc->geometry,
                       cap * pdc->geometry.length / value, colors.bg);
    }
    else // Value is less then cap
        /* Content */
        draw_content(pdc->x, pdc->geometry,
                     fit_in(value, 0, cap) * pdc->geometry.length / cap,
                     colors);

    XFlush(pdc->x.display);

    /* Draw text */
    if (pdc->text_rendering.text_count != 0)
    {
        XftDrawStringUtf8(pdc->text_rendering.xft_draw,
                          &pdc->text_rendering.ptext->font_color,
                          pdc->text_rendering.ptext->font,
                          pdc->text_rendering.ptext->pos.x,
                          pdc->text_rendering.ptext->pos.y,
                          (const FcChar8 *)pdc->text_rendering.ptext->string,
                          strlen(pdc->text_rendering.ptext->string));
    }

    XFlush(pdc->x.display);
}

/* PUBLIC Hide the window */
void hide(Display_context *pdc)
{
    if (pdc->x.mapped)
    {
        XUnmapWindow(pdc->x.display, pdc->x.window);
        pdc->x.mapped = False;
        XFlush(pdc->x.display);
    }
}
