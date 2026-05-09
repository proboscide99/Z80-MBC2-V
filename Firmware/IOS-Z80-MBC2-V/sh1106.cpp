#include "sh1106.h"
#include <Wire.h>

static void sh1106_cmd(uint8_t);
static void sh1106_data(const uint8_t *, uint8_t);

// Variabili locali
uint8_t cursor_x = 0;
uint8_t cursor_y = 0;
const Font *curFont = NULL;
uint8_t contrast = 0;
uint8_t dimTimer = 0;


// -----------------------------
// Display Init
// -----------------------------
bool sh1106_init()
{
//  Wire.begin();
//  delay(100);

  Wire.beginTransmission(SH1106_ADDR);
  if (Wire.endTransmission() != 0)
    return(false);

  contrast = SH1106_DEFAULT_CONTRAST;

  sh1106_cmd(0xAE);                                       // Display OFF
  sh1106_cmd(0xD5); sh1106_cmd(0x80);
  sh1106_cmd(0xA8); sh1106_cmd(0x3F);
  sh1106_cmd(0xD3); sh1106_cmd(0x00);
  sh1106_cmd(0x40);                                       // Start line = 0
  sh1106_cmd(0xA1);                                       // Segment remap (inverted)
  sh1106_cmd(0xC8);                                       // COM scan direction (inverted)
//  sh1106_cmd(0xA0);                                       // Segment remap normal
//  sh1106_cmd(0xC0);                                       // COM scan normal
  sh1106_cmd(0xDA); sh1106_cmd(0x12);
  sh1106_cmd(0x81); sh1106_cmd(SH1106_DEFAULT_CONTRAST);
  sh1106_cmd(0xD9); sh1106_cmd(0x22);
  sh1106_cmd(0xDB); sh1106_cmd(0x35);
  sh1106_cmd(0xA4);
  sh1106_cmd(0xA6);
  sh1106_cmd(0xAD); sh1106_cmd(0x8B);
  sh1106_cmd(0xAF);                                       // Display ON

  return(true);
}


// -----------------------------
// Funzione da chiamare su 1s
// -----------------------------
void sh1106_1s(void)
{
  if (dimTimer > 0)
  {
    --dimTimer;

    if (dimTimer == 0)
    {
      if (contrast > SH1106_MIN_CONTRAST)
      {
        sh1106_contrast(SH1106_MIN_CONTRAST);
        dimTimer = 255;
      }
      else
      {
        sh1106_cmd(0xAE);
        contrast = 0;
      }
    }
  }
}


// -----------------------------
// Imposta posizione
// -----------------------------
void sh1106_set_pos(uint8_t col, uint8_t page)
{
  uint8_t hwcol = col + SH1106_COL_OFFSET;

  sh1106_cmd(0xB0 | (page & 0x0F));
  sh1106_cmd(hwcol & 0x0F);
  sh1106_cmd(0x10 | (hwcol >> 4));
}

// -----------------------------
// Clear screen
// -----------------------------
void sh1106_clear()
{
  uint8_t empty[16];
  for (uint8_t i = 0; i < 16; i++) empty[i] = 0x00;

  for (uint8_t page = 0; page < 8; page++)
  {
    sh1106_set_pos(0, page);
    for (uint8_t block = 0; block < 8; block++)
    {
        sh1106_data(empty, 16);
    }
  }
}

// -----------------------------
// Display ON/OFF / contrast
// -----------------------------
void sh1106_contrast(uint8_t level)
{
  if (level == contrast)                                  // se non c'e' nulla da cambiare, rientra
    return;

  if (contrast == 0)                                      // se era spento,
  {
    sh1106_cmd(0xAF);                                     // comando di accensione
  }
  else if (level == 0)                                    // se era acceso e il nuovo livello e' zero, lo spegne
    sh1106_cmd(0xAE);

  contrast = level;

  sh1106_cmd(0x81);
  sh1106_cmd(contrast);                                   // imposta il contrasto

  if (contrast > 0)                                       // se il livello richiesto e' maggiore di zero,
    dimTimer = 255;                                       // imposta il timer del dimmer
}


// ===== FONT REGISTRATION =====
void sh1106_registerFont(const Font *f)
{
  curFont = f;
}

// -----------------------------
// Scrivi un carattere
// -----------------------------
void sh1106_write_char(char c)
{
  if (curFont == NULL)
    return;

  if (contrast < SH1106_DEFAULT_CONTRAST)
    sh1106_contrast(SH1106_DEFAULT_CONTRAST);

  dimTimer = 255;

  if (c < curFont->first || c >= (curFont->first + curFont->count))
    c = ' ';

  uint8_t width = curFont->width;
  if (curFont->height > 8)
    width *= 2;
  uint16_t base = (uint16_t)(c - curFont->first) * width;

  if (curFont->height <= 8)
    sh1106_data(curFont->data+base, curFont->width);
  else
  {
    sh1106_set_pos(0, 0);
    sh1106_data(curFont->data+base, curFont->width);

    sh1106_set_pos(0, 1);
    sh1106_data(curFont->data+base+curFont->width, curFont->width);
  }
}

// -----------------------------
// Scrivi una stringa
// -----------------------------
//
void sh1106_write_string(uint8_t col, uint8_t page, const char *s)
{
  sh1106_set_pos(col, page);

  while (*s)
  {
    sh1106_write_char(*s);
    col += 8;
    s++;
  }
}


// -----------------------------
// Invio comando
// -----------------------------
static void sh1106_cmd(uint8_t cmd) {
    Wire.beginTransmission(SH1106_ADDR);
    Wire.write(0x00);
    Wire.write(cmd);
    Wire.endTransmission();
}

// -----------------------------
// Invio dati
// -----------------------------
static void sh1106_data(const uint8_t *data, uint8_t len) {
    Wire.beginTransmission(SH1106_ADDR);
    Wire.write(0x40);
    for (uint8_t i = 0; i < len; i++)
        Wire.write(data[i]);
    Wire.endTransmission();
}
