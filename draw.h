
/* See LICENSE file for copyright and license details. 
 * 
 * Originally designed by the Suckless Team, modified
 * and ported to XCB by Nathan McCloskey <npgm at vt
 * dot edu>
 * */


#define FG(dc, col)  ((col)[(dc)->invert ? ColBG : ColFG])
#define BG(dc, col)  ((col)[(dc)->invert ? ColFG : ColBG])

enum { ColBG, ColFG, ColBorder, ColLast };

typedef struct {
	uint32_t x, y, w, h;
	uint8_t invert;
	xcb_connection_t *c;
	xcb_screen_t *screen;
	xcb_gcontext_t gc;
	xcb_pixmap_t canvas;
	struct {
		uint16_t ascent;
		uint16_t descent;
		uint32_t height;
		uint32_t width;
		xcb_font_t xfont;
	} font;
} DC;  /* draw context */

void drawrect(DC *dc, int16_t x, int16_t y, uint16_t w, uint16_t h, bool fill, uint32_t color);
void drawtext(DC *dc, const char *text, uint32_t col[ColLast]);
void drawtextn(DC *dc, const char *text, size_t n, uint32_t col[ColLast]);
//void eprintf(const int8_t *fmt, ...);
void freedc(DC *dc);
uint32_t getcolor(DC *dc, const char *colstr);
DC *initdc(void);
void initfont(DC *dc, const char *fontstr);
void mapdc(DC *dc, xcb_window_t win, uint16_t w, uint16_t h);
void resizedc(DC *dc, uint16_t w, uint16_t h);
int32_t textnw(DC *dc, const char *text, size_t len);
int32_t textw(DC *dc, const char *text); 
