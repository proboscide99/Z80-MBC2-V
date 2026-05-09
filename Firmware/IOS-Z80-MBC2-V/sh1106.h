#ifndef SH1106_H
#define SH1106_H

#include <stdint.h>

#define SH1106_ADDR                   0x3C
#define SH1106_COL_OFFSET             2
#define SH1106_DEFAULT_CONTRAST       0x80
#define SH1106_MIN_CONTRAST           1

typedef struct {
    uint8_t width;            // pixel per glyph (es. 6, 8, 12)
    uint8_t height;           // pixel verticali (es. 8, 16, 24)
    const uint8_t *data;      // bitmap: glyphs consecutive, ogni glyph = width * rowsPerGlyph bytes
    uint8_t first;            // codice del primo carattere (es. 0x20)
    uint8_t count;            // numero di glyph disponibili
} Font;

bool sh1106_init();
void sh1106_set_pos(uint8_t page, uint8_t col);
void sh1106_clear();
void sh1106_contrast(uint8_t);
void sh1106_write_char(char);
void sh1106_write_string(uint8_t page, uint8_t col, const char *s);
void sh1106_registerFont(const Font *);
void sh1106_1s(void);
#endif
