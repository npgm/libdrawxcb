/* See LICENSE file for copyright and license details. */
#include <locale.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xcb/xcb.h>
#include "draw.h"

#define MAX(a, b)  ((a) > (b) ? (a) : (b))
#define MIN(a, b)  ((a) < (b) ? (a) : (b))
#define DEFAULTFN  "fixed"

static bool loadfont(DC *dc, const char *fontstr);

void 
drawrect(DC *dc, int16_t x, int16_t y, uint16_t w, uint16_t h, bool fill, uint32_t color) {
	xcb_rectangle_t	rect = {x, y, w, h};
	uint32_t	val[2];
	val[0] = color;
	xcb_change_gc(dc->c, dc->gc, XCB_GC_FOREGROUND, val);
	if(fill)
		xcb_poly_fill_rectangle(dc->c, dc->canvas, dc->gc, 1, &rect);
	else
		xcb_poly_rectangle(dc->c, dc->canvas, dc->gc, 1, &rect);
}

void
drawtext(DC *dc, const char *text, uint32_t col[ColLast]) {
	char buf[BUFSIZ];
	size_t mn, n = strlen(text);

	/* shorten text if necessary */
	for(mn = MIN(n, sizeof buf); textnw(dc, text, mn) + dc->font.height/2 > dc->w; mn--)
		if(mn == 0)
			return;
	memcpy(buf, text, mn);
	if(mn < n)
		for(n = MAX(mn-3, 0); n < mn; buf[n++] = '.');

//	drawrect(dc, 0, 0, dc->w, dc->h, true, BG(dc, col));
	drawtextn(dc, buf, mn, col);
}

void
drawtextn(DC *dc, const char *text, size_t n, uint32_t col[ColLast]) {
	xcb_void_cookie_t		cookie;
	xcb_generic_error_t		*error;
	uint32_t			val[3];
	uint32_t			mask;

	int x = dc->x + dc->font.height/2;
	int y = dc->y + dc->font.ascent+1;

	mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND | XCB_GC_FONT;

	val[0] = FG(dc, col);
	val[1] = BG(dc, col);
	val[2] = dc->font.xfont;

	cookie = xcb_change_gc_checked(dc->c, dc->gc, mask, val);
	
	if(!((error = xcb_request_check(dc->c, cookie)))) {
		cookie = xcb_image_text_8_checked(dc->c, n, dc->canvas, dc->gc, x, y, text);
		if(!((error = xcb_request_check(dc->c, cookie))))
			return;
	}

	fprintf(stderr, "errror could not complete drawtext request");
	free(error);
}

void
freedc(DC *dc) {
	if(dc->font.xfont)
//		XFreeFont(dc->dpy, dc->font.xfont);
	if(dc->canvas)
		xcb_free_pixmap(dc->c, dc->canvas);
	xcb_free_gc(dc->c, dc->gc);
	xcb_disconnect(dc->c);
	free(dc);
}

uint32_t 
getcolor(DC *dc, const char *colstr){
	unsigned int  r = 0, g = 0, b = 0; 
	uint32_t pixel = 0;
	xcb_alloc_color_reply_t		*reply;
	/* convert 24 bit color values to 48 bit color values 
	 * with assistance from bspwm by Baskerville
	 * https://github.com/baskerville/bspwm */
	fprintf(stderr, "color string: %s\n", colstr+1);
	if (sscanf(colstr+1, "%2x%2x%2x", &r, &g, &b) == 3) {
//		convert color to 48 bit 0x23 * 0x101 = 0x2323
		fprintf(stderr, "r: %x, g: %x, b: %x\n", r, g, b);
		r *= 0x101;
		g *= 0x101;
		b *= 0x101;

		if((reply = xcb_alloc_color_reply(dc->c, xcb_alloc_color(dc->c, dc->screen->default_colormap, r, g, b), NULL))) {
			pixel = reply->pixel;
			fprintf(stderr, "getcolor success\n");
			free(reply);
		}
	}
	return pixel;
}

DC *
initdc(void) {
	DC 		*dc;
	uint32_t	val[3];
	uint32_t	mask;

	if (!(dc = calloc(1, sizeof(DC))))
		fprintf(stderr, "ERROR: Could not allocate memory for draw context\n");
	if (!(dc->c = xcb_connect (NULL, NULL)))
		fprintf(stderr, "ERROR: Could not connect to X\n");

	dc->screen = xcb_setup_roots_iterator(xcb_get_setup(dc->c)).data;

	dc->gc = xcb_generate_id(dc->c);
	mask = XCB_GC_LINE_STYLE | XCB_GC_CAP_STYLE | XCB_GC_JOIN_STYLE;
	
	val[0] = XCB_LINE_STYLE_SOLID;
	val[1] = XCB_CAP_STYLE_BUTT;
	val[2]	= XCB_JOIN_STYLE_MITER;
	
	xcb_create_gc(dc->c, dc->gc, dc->screen->root, mask, val);

	return dc;
}

void 
initfont(DC *dc, const char *fontstr) {
	if(!loadfont(dc, fontstr ? fontstr : DEFAULTFN)) {
		if(fontstr != NULL)
			fprintf(stderr, "cannot load font '%s'\n", fontstr);
		if(fontstr == NULL || !loadfont(dc, DEFAULTFN))
			fprintf(stderr, "cannot load font '%s'\n", DEFAULTFN);
	}
	dc->font.height = dc->font.ascent + dc->font.descent;
}

static bool
loadfont(DC *dc, const char* fontstr) {
	xcb_font_t 		font;
	xcb_void_cookie_t 	cookie;
	xcb_generic_error_t 	*error;
	xcb_query_font_cookie_t	fookie;
	xcb_query_font_reply_t	*reply;

	font = xcb_generate_id(dc->c);
	cookie = xcb_open_font_checked(dc->c, font, strlen(fontstr), fontstr);

	if(!((error = xcb_request_check(dc->c, cookie)))) {
		fookie = xcb_query_font(dc->c, font);
		if ((reply = xcb_query_font_reply(dc->c, fookie, NULL))) {
			dc->font.ascent = reply->font_ascent;
			dc->font.descent = reply->font_descent;
			dc->font.width = reply->max_bounds.character_width;
			dc->font.xfont = font;
			return true;
		}
	}
	fprintf(stderr, "cannot load font");
	free(error);
	return false;
}

void
mapdc(DC *dc, xcb_window_t win, uint16_t w, uint16_t h) {
	xcb_generic_error_t *error;
	xcb_void_cookie_t cookie = xcb_copy_area_checked(dc->c, dc->canvas, win, dc->gc, 0, 0, 0, 0, w, h);
	if ((error = xcb_request_check(dc->c, cookie))) {
		fprintf(stderr, "mapdc failure\n");
		free(error);
	}
	else
		fprintf(stderr, "mapdc success\n");
	xcb_flush(dc->c);
}

void
resizedc(DC *dc, uint16_t w, uint16_t h) {
	xcb_generic_error_t *error;
	xcb_void_cookie_t cookie;

	dc->w = w;
	dc->h = h;
	dc->canvas = xcb_generate_id(dc->c);
	cookie = xcb_create_pixmap_checked(dc->c, dc->screen->root_depth, dc->canvas, dc->screen->root, w, h);	
	if ((error = xcb_request_check(dc->c, cookie))) {
		fprintf(stderr, "resizedc failure\n");
		free(error);
	}
	else
		fprintf(stderr, "resizedc success\n");
}

int32_t
textnw(DC *dc, const char *text, size_t len) {
	xcb_query_text_extents_cookie_t	cookie;
	xcb_query_text_extents_reply_t	*reply;
	int32_t				width;
	xcb_char2b_t			*string;

	string = calloc(1, sizeof(xcb_char2b_t));
	string->byte1 = 0;
	string->byte2 = 0;

	memcpy(&string->byte2, text, sizeof(char));
	cookie = xcb_query_text_extents(dc->c, dc->font.xfont, len, string);
	if ((reply = xcb_query_text_extents_reply(dc->c, cookie, NULL))) {
		width = reply->overall_width;
		free(reply);
		return width;
	}
	
	return 0;
}

int32_t
textw(DC *dc, const char *text) {
	return textnw(dc, text, strlen(text)) + dc->font.height;
}
