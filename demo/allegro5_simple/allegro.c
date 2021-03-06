/*
    Copyright (c) 2016 Micha Mettke

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1.  The origin of this software must not be misrepresented; you must not
        claim that you wrote the original software. If you use this software
        in a product, an acknowledgment in the product documentation would be
        appreciated but is not required.
    2.  Altered source versions must be plainly marked as such, and must not be
        misrepresented as being the original software.
    3.  This notice may not be removed or altered from any source distribution.
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <math.h>

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>

/* macros */
#define DEMO_DO_NOT_DRAW_IMAGES
#include "../../zahnrad.h"
#include "../demo.c"

static void
die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputs("\n", stderr);
    exit(EXIT_FAILURE);
}

static void
draw(struct zr_context *ctx, int width, int height)
{
    const struct zr_command *cmd;
    zr_foreach(cmd, ctx) {
        switch (cmd->type) {
        case ZR_COMMAND_NOP: break;
        case ZR_COMMAND_SCISSOR: {
            const struct zr_command_scissor *s = zr_command(scissor, cmd);
            al_set_clipping_rectangle(s->x-1,s->y-1,s->w+2, s->h+2);
        } break;
        case ZR_COMMAND_LINE: {
            const struct zr_command_line *l = zr_command(line, cmd);
            al_draw_line(l->begin.x, l->begin.y, l->end.x, l->end.y,
                al_map_rgba(l->color.r, l->color.g, l->color.b, l->color.a), 1.0f);
        } break;
        case ZR_COMMAND_RECT: {
            const struct zr_command_rect *r = zr_command(rect, cmd);
            al_draw_filled_rounded_rectangle(r->x, r->y, r->x + r->w, r->y + r->h,
                r->rounding, r->rounding,
                al_map_rgba(r->color.r, r->color.g, r->color.b, r->color.a));
        } break;
        case ZR_COMMAND_CIRCLE: {
            const struct zr_command_circle *c = zr_command(circle, cmd);
            al_draw_filled_circle(c->x + c->w/2, c->y + c->w/2, c->w/2,
                al_map_rgba(c->color.r, c->color.g, c->color.b, c->color.a));
        } break;
        case ZR_COMMAND_TRIANGLE: {
            const struct zr_command_triangle *t = zr_command(triangle, cmd);
            al_draw_filled_triangle(t->a.x, t->a.y, t->b.x, t->b.y, t->c.x, t->c.y,
                al_map_rgba(t->color.r, t->color.g, t->color.b, t->color.a));
        } break;
        case ZR_COMMAND_TEXT: {
            const struct zr_command_text *t = zr_command(text, cmd);
            ALLEGRO_FONT *font = t->font->userdata.ptr;
            al_draw_text(font, al_map_rgba(t->foreground.r, t->foreground.g, t->foreground.g, t->foreground.a),
                    t->x, t->y, ALLEGRO_ALIGN_LEFT, t->string);
        } break;
        case ZR_COMMAND_CURVE: {
            unsigned int i_step;
            float t_step;
            const struct zr_command_curve *q = zr_command(curve, cmd);
            struct zr_vec2i last = q->begin;
            struct zr_vec2i p1 = q->begin;
            struct zr_vec2i p2 = q->ctrl[0];
            struct zr_vec2i p3 = q->ctrl[1];
            struct zr_vec2i p4 = q->end;
            t_step = 1.0f/(float)22.0f;
            for (i_step = 1; i_step <= 22.0f; ++i_step) {
                float t = t_step * (float)i_step;
                float u = 1.0f - t;
                float w1 = u*u*u;
                float w2 = 3*u*u*t;
                float w3 = 3*u*t*t;
                float w4 = t * t *t;
                float x = w1 * p1.x + w2 * p2.x + w3 * p3.x + w4 * p4.x;
                float y = w1 * p1.y + w2 * p2.y + w3 * p3.y + w4 * p4.y;
                al_draw_line(last.x, last.y, x, y,
                    al_map_rgba(q->color.r, q->color.g, q->color.b, q->color.a), 1.0f);
                last.x = (short)x; last.y = (short)y;
            }
        } break;
        case ZR_COMMAND_ARC:
        case ZR_COMMAND_IMAGE:
        default: break;
        }
    }
    zr_clear(ctx);
    al_set_clipping_rectangle(0,0,width, height);
}

static size_t
font_get_width(zr_handle handle, float height, const char *text, size_t len)
{
    /* <heavy sign> allegro does not support text width calculation
     * for strings that are not zero terminated... Therefore I have to
     * allocate a string every time.
     * OPT: maybe use a buffer to copy into? */
    const ALLEGRO_FONT *font = handle.ptr;
    ALLEGRO_USTR *str = al_ustr_new_from_buffer(text, len);
    size_t width = (size_t)al_get_ustr_width(font, str);
    UNUSED(height);
    al_ustr_free(str);
    return width;
}

static void
input_key(struct zr_context *ctx, ALLEGRO_EVENT *evt, int down)
{
    int sym = evt->keyboard.keycode;
    if (sym == ALLEGRO_KEY_RSHIFT || sym == ALLEGRO_KEY_LSHIFT)
        zr_input_key(ctx, ZR_KEY_SHIFT, down);
    else if (sym == ALLEGRO_KEY_DELETE)
        zr_input_key(ctx, ZR_KEY_DEL, down);
    else if (sym == ALLEGRO_KEY_ENTER)
        zr_input_key(ctx, ZR_KEY_ENTER, down);
    else if (sym == ALLEGRO_KEY_TAB)
        zr_input_key(ctx, ZR_KEY_TAB, down);
    else if (sym == ALLEGRO_KEY_BACKSPACE)
        zr_input_key(ctx, ZR_KEY_BACKSPACE, down);
    else if (sym == ALLEGRO_KEY_LEFT)
        zr_input_key(ctx, ZR_KEY_LEFT, down);
    else if (sym == ALLEGRO_KEY_RIGHT)
        zr_input_key(ctx, ZR_KEY_RIGHT, down);
    else if (sym == ALLEGRO_KEY_C)
        zr_input_key(ctx, ZR_KEY_COPY, down && evt->keyboard.modifiers & ALLEGRO_KEYMOD_CTRL);
    else if (sym == ALLEGRO_KEY_V)
        zr_input_key(ctx, ZR_KEY_PASTE, down && evt->keyboard.modifiers & ALLEGRO_KEYMOD_CTRL);
    else if (sym == ALLEGRO_KEY_X)
        zr_input_key(ctx, ZR_KEY_CUT, down && evt->keyboard.modifiers & ALLEGRO_KEYMOD_CTRL);
}

static void
input_button(struct zr_context *ctx, ALLEGRO_EVENT *evt, int down)
{
    const int x = evt->mouse.x;
    const int y = evt->mouse.y;
    if (evt->mouse.button == 1)
        zr_input_button(ctx, ZR_BUTTON_LEFT, x, y, down);
    if (evt->mouse.button == 2)
        zr_input_button(ctx, ZR_BUTTON_RIGHT, x, y, down);
}

static void* mem_alloc(zr_handle unused, size_t size)
{UNUSED(unused); return calloc(1, size);}
static void mem_free(zr_handle unused, void *ptr)
{UNUSED(unused); free(ptr);}

int
main(int argc, char *argv[])
{
    /* Platform */
    const char *font_path;
    int width = 0, height = 0;
    int running = 1;

    ALLEGRO_DISPLAY *display;
    ALLEGRO_EVENT_QUEUE *queue;
    ALLEGRO_FONT *font;
    struct demo gui;

    font_path = argv[1];
    if (argc < 2)
        die("Missing TTF Font file argument!");

    /* Allegro */
    al_init();
    al_init_font_addon();
    al_init_ttf_addon();
    al_install_keyboard();
    al_install_mouse();
    al_init_primitives_addon();
    display = al_create_display(WINDOW_WIDTH, WINDOW_HEIGHT);
    al_set_window_title(display, "Zahnrad");
    queue = al_create_event_queue();
    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_mouse_event_source());
    font = al_load_ttf_font(font_path, 14, 0);
    if (!font) die("Failed to load font: %s\n", font_path);

    {
        /* GUI */
        struct zr_user_font usrfnt;
        struct zr_allocator alloc;
        alloc.userdata.ptr = NULL;
        alloc.alloc = mem_alloc;
        alloc.free = mem_free;

        usrfnt.userdata = zr_handle_ptr(font);
        usrfnt.width = font_get_width;
        usrfnt.height = al_get_font_line_height(font);
        zr_init(&gui.ctx, &alloc, &usrfnt);
    }

    while (running) {
        /* Input */
        ALLEGRO_EVENT evt;
        zr_input_begin(&gui.ctx);
        while (al_get_next_event(queue, &evt)) {
            if (evt.type == ALLEGRO_EVENT_DISPLAY_CLOSE) goto cleanup;
            else if (evt.type == ALLEGRO_EVENT_KEY_UP && evt.keyboard.display == display)
                input_key(&gui.ctx, &evt, zr_false);
            else if (evt.type == ALLEGRO_EVENT_KEY_DOWN && evt.keyboard.display == display)
                input_key(&gui.ctx, &evt, zr_true);
            else if (evt.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN)
                input_button(&gui.ctx, &evt, zr_true);
            else if (evt.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP)
                input_button(&gui.ctx, &evt, zr_false);
            else if (evt.type == ALLEGRO_EVENT_MOUSE_AXES) {
                zr_input_motion(&gui.ctx, evt.mouse.x, evt.mouse.y);
            } else if (evt.type == ALLEGRO_EVENT_KEY_CHAR) {
                if (evt.keyboard.display == display)
                    if (evt.keyboard.unichar > 0 && evt.keyboard.unichar < 0x10000)
                        zr_input_unicode(&gui.ctx, (zr_rune)evt.keyboard.unichar);
            }
        }
        zr_input_end(&gui.ctx);

        /* GUI */
        width = al_get_display_width(display);
        height = al_get_display_height(display);
        running = run_demo(&gui);

        /* Draw */
        al_clear_to_color(al_map_rgba_f(0.2f, 0.2f, 0.2f, 1.0f));
        draw(&gui.ctx, width, height);
        al_flip_display();
    }

cleanup:
    /* Cleanup */
    if (queue) al_destroy_event_queue(queue);
    if (display) al_destroy_display(display);
    zr_free(&gui.ctx);
    return 0;
}

