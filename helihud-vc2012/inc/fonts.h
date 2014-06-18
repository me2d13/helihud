#ifndef _fonts_h_
#define _fonts_h_

#include "XPLMGraphics.h"

typedef struct HUDFontProperties {
  char *fileName;
  char firstChar;
  XPLMTextureID texId;
  int dispListBase;
  int rowsCount;
  int colsCount;
  int charWidth;
  int charHeight;
  int spacing;
  int texSize;
}
HUDFontProperties;


void LoadFontTextures(void);
void CreateHUDFont(HUDFontProperties *f);
void DrawHUDText(const char *pValue, HUDFontProperties *f, int pX, int pY, char pAllign);
void DrawHUDNumber(int pValue, HUDFontProperties *f, int pDigits, int pX, int pY, char pAllign);
void DrawTexts(void );
void InitFonts(void);
int LoadHUDFontTexture(HUDFontProperties *f);
HUDFontProperties *GetBigFont(void);
HUDFontProperties *GetSmallFont(void);
int getTextWidth(HUDFontProperties *f, int numberOfChars);
void SetGLText( void );

#endif