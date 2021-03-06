#if IBM
#include <windows.h>
#define _USE_MATH_DEFINES
#endif
#if LIN
#include <GL/gl.h>
#else
#define TRUE 1
#define FALSE 0
#if APL
#include <Carbon/Carbon.h>
#endif
#if __GNUC__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#endif
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "drawing.h"
#include "fonts.h"
#include "XPLMDisplay.h"
#include "XPLMGraphics.h"
#include "BitmapSupport.h"
#include "data_access.h"


int initGlResources(void ) {
  InitFonts();
  return 0;
}

int destroyGlResources( void ) {
  return 0;
}

void TranslateToCenter(void) {
  int screen_width;
  int screen_height;
  XPLMGetScreenSize(&screen_width, &screen_height);
  HudConfig* lConfig = getHudConfig();
  if (lConfig->centered)
    glTranslated(screen_width / 2, screen_height / 2, 0);
  else
    glTranslated(lConfig->x, lConfig->y, 0);
}

void DrawCenterBox(void ) {
  float lSize = getHudConfig()->size;
  /* Set the color to magenta.  Note that magenta is NOT naturally
   * transparent when pluings draw!!  The magenta=transparent
   * convention is provided by X-Plane for some bitmaps only.  */
  glColor3fv(getHudConfig()->clCenterBox);

  glTranslated(- CENTER_BOX_SIZE*lSize / 2, - CENTER_BOX_SIZE*lSize / 2, 0);
  glBegin(GL_LINES);
  // left bottom
  glVertex2i(0,0);
  glVertex2f(CENTER_BOX_LINE_LENGTH*lSize, 0);
  glVertex2i(0,0);
  glVertex2f(0, CENTER_BOX_LINE_LENGTH*lSize);
  // left top
  glVertex2f(0, CENTER_BOX_SIZE*lSize);
  glVertex2f(CENTER_BOX_LINE_LENGTH*lSize, CENTER_BOX_SIZE*lSize);
  glVertex2f(0, CENTER_BOX_SIZE*lSize);
  glVertex2f(0, CENTER_BOX_SIZE*lSize-CENTER_BOX_LINE_LENGTH*lSize);
  // right bottom
  glVertex2f(CENTER_BOX_SIZE*lSize,0);
  glVertex2f(CENTER_BOX_SIZE*lSize - CENTER_BOX_LINE_LENGTH*lSize, 0);
  glVertex2f(CENTER_BOX_SIZE*lSize,0);
  glVertex2f(CENTER_BOX_SIZE*lSize, CENTER_BOX_LINE_LENGTH*lSize);
  // right top
  glVertex2f(CENTER_BOX_SIZE*lSize, CENTER_BOX_SIZE*lSize);
  glVertex2f(CENTER_BOX_SIZE*lSize - CENTER_BOX_LINE_LENGTH*lSize, CENTER_BOX_SIZE*lSize);
  glVertex2f(CENTER_BOX_SIZE*lSize, CENTER_BOX_SIZE*lSize);
  glVertex2f(CENTER_BOX_SIZE*lSize, CENTER_BOX_SIZE*lSize-CENTER_BOX_LINE_LENGTH*lSize);
  glEnd();
  // return axis
  glTranslatef(CENTER_BOX_SIZE*lSize / 2, CENTER_BOX_SIZE*lSize / 2, 0);
}

void DrawNoseBox(float pitch, float roll) {
  float lx;
  float ly;
  HudConfig* lC = getHudConfig();
  float lSize = lC->size;
  lx = (roll / lC->rngRollDeg) * NOSE_AREA_SIZE*lSize / 2;
  if (lx > NOSE_AREA_SIZE*lSize / 2)
    lx = NOSE_AREA_SIZE*lSize / 2;
  if (lx < - NOSE_AREA_SIZE*lSize / 2)
    lx = - NOSE_AREA_SIZE*lSize / 2;
  ly = (pitch / lC->rngPitchDeg) * NOSE_AREA_SIZE*lSize / 2;
  if (ly > NOSE_AREA_SIZE*lSize / 2)
    ly = NOSE_AREA_SIZE*lSize / 2;
  if (ly < - NOSE_AREA_SIZE*lSize / 2)
    ly = - NOSE_AREA_SIZE*lSize / 2;
  glTranslatef(lx, ly, 0);
  glColor3fv(lC->clCenterBox);
  glBegin(GL_LINES);
  glVertex2i(0,0);
  glVertex2i(NOSE_BOX_SIZE*lSize, 0);
  glVertex2i(NOSE_BOX_SIZE*lSize, 0);
  glVertex2i(NOSE_BOX_SIZE*lSize, NOSE_BOX_SIZE*lSize);
  glVertex2i(NOSE_BOX_SIZE*lSize, NOSE_BOX_SIZE*lSize);
  glVertex2i(0, NOSE_BOX_SIZE*lSize);
  glVertex2i(0, NOSE_BOX_SIZE*lSize);
  glVertex2i(0, 0);
  glEnd();
  // return axis
  glTranslatef(-lx, -ly, 0);
}

void DrawCircle(int radius) {
  double angle;
  int i;
  glBegin(GL_LINE_LOOP);
  for(i = 0; i < CIRCLE_LINES_COUNT; i++)
  {
    angle = i*2*M_PI/CIRCLE_LINES_COUNT;
    glVertex2f((cos(angle) * radius), (sin(angle) * radius));
  }
  glEnd();
}

void DrawMovementArrow(float trueHeading, float vx, float vy, float vz) {
  float speed; // in kias (or m/s)
  float angle;
  float draw_speed; // in pixels
  float lColor[3];
  HudConfig* lC = getHudConfig();
  float lSize = lC->size;
  // todo: add wind to vx and vy
  vz = -vz;
  speed = sqrt(vx*vx + vz*vz);
  // gs
  if (speed < lC->rngMovementArrowMs)
  {
    SetGLText(); // turn on blending
    DrawHUDNumber((int ) convertSpeed(speed, SU_MPS, getHudConfig()->gsUnits) , GetSmallFont(), 3, GROUND_SPEED_POS*lSize, -GROUND_SPEED_POS*lSize, 1);
    if (getHudConfig()->showUnits)
      DrawHUDText(getUnitsLabel(getHudConfig()->gsUnits), GetSmallFont(),
                  GROUND_SPEED_POS*lSize, -GROUND_SPEED_POS*lSize-GetSmallFont()->charHeight-3, 1);
    // turn off blending
    XPLMSetGraphicsState(0, 0, 0, 0, 0, 0, 0);
  }
  if (speed < 0.5)
  {
    speed = 0;
    angle = 0;
    return;
  }
  angle = asin(vz/speed) * 180 / M_PI;
  // convert angle from x-axis to angle from Z axis (heading 0)
  if (vx >=0)
    // 1st & 2nd quandrant
    angle = 90 - angle;
  else
    //3rd & 4th
    angle = 270 + angle;
  // take heading into account
  angle -= (trueHeading);
  // translation already done, we have center in screen center
  draw_speed = MOVEMENT_ARROW_RADIUS_PX*lSize * speed / lC->rngMovementArrowMs;
  if (draw_speed > MOVEMENT_ARROW_RADIUS_PX*lSize)
    draw_speed = MOVEMENT_ARROW_RADIUS_PX*lSize;
  glRotatef(-angle, 0, 0, 1);
  interColor(lC->clMovementBackward, lC->clMovementForward, lColor, (cos(angle*M_PI/180.0f)+1.0f)/2.0f);
  glColor3fv(lColor);
  glBegin(GL_LINES);
  if (speed <= lC->rngMovementArrowMs)
  {
    glVertex2f(0,0);
    glVertex2f(0,draw_speed);
  }
  // arrow
  glVertex2f(0,draw_speed);
  glVertex2f(-MOVEMENT_ARROW_LEG_SIZE*lSize, draw_speed-2*MOVEMENT_ARROW_LEG_SIZE*lSize);
  glVertex2f(0,draw_speed);
  glVertex2f(MOVEMENT_ARROW_LEG_SIZE*lSize, draw_speed-2*MOVEMENT_ARROW_LEG_SIZE*lSize);
  glEnd();
  glRotatef(angle, 0, 0, 1);
#ifdef DEBUG

  float color[] = { 1.0, 1.0, 1.0 };  /* RGB White */
  char buffer[255];
  sprintf(buffer, "True heading: %f", trueHeading);
  XPLMDrawString(color, -200, 200, buffer, NULL, xplmFont_Basic);
  sprintf(buffer, "Angle: %f", angle);
  XPLMDrawString(color, -200, 180, buffer, NULL, xplmFont_Basic);
  sprintf(buffer, "Vx: %f", vx);
  XPLMDrawString(color, -200, 160, buffer, NULL, xplmFont_Basic);
  sprintf(buffer, "Vz: %f", vz);
  XPLMDrawString(color, -200, 140, buffer, NULL, xplmFont_Basic);
#endif
}

void DrawWind(float pHeading, float pSpeed, float pAcfHeading) {
  float angle;
  float lSize = getHudConfig()->size;
  angle = pHeading-pAcfHeading;
  glColor3fv(getHudConfig()->clWindArrow);
  glRotatef(-angle, 0, 0, 1);
  glBegin(GL_LINE_LOOP);
  // arrow
  glVertex2f(0,WIND_ARROW_RADIUS_PX*lSize);
  glVertex2f(-WIND_ARROW_SIZE*lSize, (WIND_ARROW_RADIUS_PX+WIND_ARROW_SIZE)*lSize);
  glVertex2f(+WIND_ARROW_SIZE*lSize, (WIND_ARROW_RADIUS_PX+WIND_ARROW_SIZE)*lSize);
  glVertex2f(0,WIND_ARROW_RADIUS_PX*lSize);
  glEnd();
  glRotatef(angle, 0, 0, 1);
  // speed
  SetGLText();
  DrawHUDNumber((int ) (convertSpeed(pSpeed, SU_KNOTS, getHudConfig()->wsUnits) + 0.5f) , GetSmallFont(), 3, -WIND_SPEED_POS*lSize, -WIND_SPEED_POS*lSize, 1);
  if (getHudConfig()->showUnits)
    DrawHUDText(getUnitsLabel(getHudConfig()->wsUnits), GetSmallFont(),
                -WIND_SPEED_POS*lSize, -WIND_SPEED_POS*lSize-GetSmallFont()->charHeight-3, 1);
  //DrawHUDNumber((int ) getViewIsExternal() , GetSmallFont(), 3, 0, 100, 0);
  XPLMSetGraphicsState(0, 0, 0, 0, 0, 0, 0); // turn off blending
#ifdef DEBUG

  float color[] = { 1.0, 1.0, 1.0 };
  char buffer[255];
  sprintf(buffer, "Wind heading: %f", pHeading);
  XPLMDrawString(color, -200, 120, buffer, NULL, xplmFont_Basic);
  sprintf(buffer, "Wind speed: %f", pSpeed);
  XPLMDrawString(color, -200, 100, buffer, NULL, xplmFont_Basic);
#endif
}


void DrawVerticalSpeedIndicator(float vv) {
  float y_pos;
  float lColor[3];
  HudConfig* lConfig = getHudConfig();
  float lSize = lConfig->size;
  if (lConfig->visVsiBall) {
    y_pos = (vv / lConfig->rngVsiBallFpm) * VSI_CIRCLE_RANGE_PX*lSize;
    if (y_pos > VSI_RANGE_PX*lSize)
      y_pos = VSI_CIRCLE_RANGE_PX*lSize;
    else if (y_pos < -VSI_CIRCLE_RANGE_PX*lSize)
      y_pos = -VSI_CIRCLE_RANGE_PX*lSize;
    if (y_pos < 0)
      interColor(lConfig->clVSBallMiddle, lConfig->clVSBallLow, lColor, -y_pos/(VSI_CIRCLE_RANGE_PX*lSize));
    else
      interColor(lConfig->clVSBallMiddle, lConfig->clVSBallHigh, lColor, y_pos/(VSI_CIRCLE_RANGE_PX*lSize));
    glColor3fv(lColor);
    glTranslatef(0, y_pos, 0);
    DrawCircle(VSI_RADIUS_PX*lSize);
    // return axis
    glTranslatef(0, -y_pos, 0);
  }
  if (lConfig->visVsi) {
      // shift VSI indicator
      y_pos = (vv / lConfig->rngVsiFpm) * VSI_RANGE_PX*lSize;
      if (y_pos > VSI_RANGE_PX*lSize)
          y_pos = VSI_RANGE_PX*lSize;
      else if (y_pos < -VSI_RANGE_PX*lSize)
          y_pos = -VSI_RANGE_PX*lSize;
      glTranslatef(-lSize/2, y_pos, 0);
      // draw box
      glColor3fv(getHudConfig()->clVSI);
      HUDFontProperties *lSF = GetSmallFont();
      int lTextWidth = getTextWidth(lSF, 4);
      glBegin(GL_LINE_LOOP);
      glVertex2f(0, lSF->charHeight / 2 + 2); // upper left
      glVertex2f(lTextWidth+5, lSF->charHeight / 2 + 2); // upper right
      glVertex2f(lTextWidth+15, 0); // right arrow
      glVertex2f(lTextWidth+5, -lSF->charHeight / 2 - 2); // lower right
      glVertex2f(0, -lSF->charHeight / 2 - 2); // lower left
      glVertex2f(0, lSF->charHeight / 2 + 2); // upper left
      glEnd();
      // draw number
      SetGLText();
      DrawHUDNumber((int )abs(convertSpeed(vv, SU_FPM, lConfig->vsUnits)), lSF, 4, lTextWidth+3, -lSF->charHeight / 2, 1);
      if (lConfig->showUnits)
          DrawHUDText(getUnitsLabel(lConfig->vsUnits), lSF,
                      lTextWidth+3, -lSF->charHeight / 2 - lSF->charHeight-3, 1);
      XPLMSetGraphicsState(0, 0, 0, 0, 0, 0, 0); // turn off blending
      // zero arrow
      glTranslatef(0, -y_pos, 0);
      glBegin(GL_LINE_LOOP);
      glVertex2f(lTextWidth+15, 0); // right arrow
      glVertex2f(lTextWidth+25, -lSF->charHeight / 2 - 2); // lower right
      glVertex2f(lTextWidth+25, +lSF->charHeight / 2 + 2); // upper right
      glVertex2f(lTextWidth+15, 0); // right arrow
      glEnd();
      // return axis
      glTranslatef(lSize/2, 0, 0);
  }
}

void DrawSpeedIndicator(float v) {
  float y_pos;
  HudConfig* lC = getHudConfig();
  float lSize = lC->size;
  // shift IAS indicator
  y_pos = (v / lC->rngIasKt) * IAS_RANGE_PX*lSize;
  if (y_pos > IAS_RANGE_PX*lSize)
    y_pos = IAS_RANGE_PX*lSize;
  else if (y_pos < -IAS_RANGE_PX*lSize)
    y_pos = -IAS_RANGE_PX*lSize;
  glTranslatef(lSize/2, y_pos, 0);
  // draw box
  HUDFontProperties *lSF = GetSmallFont();
  int lTextWidth = getTextWidth(lSF, 3);
  glColor3fv(lC->clIAS);
  glBegin(GL_LINE_LOOP);
  glVertex2f(0, lSF->charHeight / 2 + 2); // upper right
  glVertex2f(-lTextWidth-5, lSF->charHeight / 2 + 2); // upper left
  glVertex2f(-lTextWidth-15, 0); // left arrow
  glVertex2f(-lTextWidth-5, -lSF->charHeight / 2 - 2); // lower left
  glVertex2f(0, -lSF->charHeight / 2 - 2); // lower right
  glVertex2f(0, lSF->charHeight / 2 + 2); // upper right
  glEnd();
  // draw number
  SetGLText(); // turn on blending
  DrawHUDNumber((int )abs(convertSpeed(v, SU_KNOTS, getHudConfig()->iasUnits)), lSF, 3, -3, -lSF->charHeight / 2, 1);
  if (getHudConfig()->showUnits)
    DrawHUDText(getUnitsLabel(getHudConfig()->iasUnits), lSF,
                -3, -lSF->charHeight * 1.5f -3, 1);
  XPLMSetGraphicsState(0, 0, 0, 0, 0, 0, 0); // turn off blending
  // zero arrow
  glTranslatef(0, -y_pos, 0);
  glBegin(GL_LINE_LOOP);
  glVertex2f(-lTextWidth-15, 0); // right arrow
  glVertex2f(-lTextWidth-25, -lSF->charHeight / 2 - 2); // lower right
  glVertex2f(-lTextWidth-25, +lSF->charHeight / 2 + 2); // upper right
  glVertex2f(-lTextWidth-15, 0); // right arrow
  glEnd();
  // return axis
  glTranslatef(-lSize/2, 0, 0);
}


void DrawLandingBars(float radarAltitude) {
  HudConfig* lC = getHudConfig();
  float lColor[3];



  float lSize = lC->size;
  if (radarAltitude <= lC->rngLandingBarsM)
  {
    float pos;
    pos = (radarAltitude / lC->rngLandingBarsM) * LANDING_BARS_RANGE_PX*lSize;
    interColor(lC->clLandingBarsLow, lC->clLandingBarsHigh, lColor, radarAltitude / lC->rngLandingBarsM);
    glColor3fv(lColor);
    glBegin(GL_LINES);
    // left
    glVertex2f(-pos, -LANDING_BARS_SIZE*lSize/2);
    glVertex2f(-pos,  LANDING_BARS_SIZE*lSize/2);
    // right
    glVertex2f(pos, -LANDING_BARS_SIZE*lSize/2);
    glVertex2f(pos,  LANDING_BARS_SIZE*lSize/2);
    glEnd();
  }
}

void DrawBalanceIndicator(float pBalInd, float pYawStr) {
  HudConfig* lConfig = getHudConfig();
  if (!lConfig->visBalInd && !lConfig->visYawStr)
    return;
  float lBalPos;
  float lYawPos;
  float lSize = lConfig->size;
  // move down on Y axis
  glTranslatef(0, -BAL_IND_RANGE_POS*lSize, 0);
  if (1) { // draw clipping box
      glColor3fv(lConfig->clBalInd);
      float x = (BAL_IND_RANGE_PX+BAL_IND_RADIUS_PX*1.2f)*lSize;
      float y = BAL_IND_RADIUS_PX*1.2f*lSize; // 1.2 to have slight space to have ball inside
      float ol = BAL_IND_RADIUS_PX*lSize; // x overlap on side indicator
      //left
      glBegin(GL_LINE_STRIP);
      glVertex2f(-x+ol, -y); // upper 
      glVertex2f(-x, -y); 
      glVertex2f(-x, y); // lower 
      glVertex2f(-x+ol, y); 
      glEnd();
      //right
      glBegin(GL_LINE_STRIP);
      glVertex2f(x-ol, -y); // upper 
      glVertex2f(x, -y); 
      glVertex2f(x, y); // lower 
      glVertex2f(x-ol, y); 
      glEnd();
      glBegin(GL_LINES);
      // top
      glVertex2f(0, -y-ol);
      glVertex2f(0,  -y);
      // bottom
      glVertex2f(0, y);
      glVertex2f(0, y+ol);
      glEnd();
  }
  if (lConfig->visBalInd) {
    lBalPos = (pBalInd / lConfig->rngBalInd) * BAL_IND_RANGE_PX*lSize;
    if (lBalPos > BAL_IND_RANGE_PX*lSize)
      lBalPos = BAL_IND_RANGE_PX*lSize;
    else if (lBalPos < -BAL_IND_RANGE_PX*lSize)
      lBalPos = -BAL_IND_RANGE_PX*lSize;
    glColor3fv(lConfig->clBalInd);
    glTranslatef(-lBalPos, 0, 0);
    DrawCircle(BAL_IND_RADIUS_PX*lSize);
    // return axis
    glTranslatef(lBalPos, 0, 0);
  }
  if (lConfig->visYawStr) {
    lYawPos = (pYawStr / lConfig->rngYawStr) * BAL_IND_RANGE_PX*lSize;
    if (lYawPos > BAL_IND_RANGE_PX*lSize)
      lYawPos = BAL_IND_RANGE_PX*lSize;
    else if (lYawPos < -BAL_IND_RANGE_PX*lSize)
      lYawPos = -BAL_IND_RANGE_PX*lSize;
    glColor3fv(lConfig->clBalInd);
    glBegin(GL_LINES);
    glVertex2f(lYawPos, -BAL_IND_RADIUS_PX*lSize);
    glVertex2f(lYawPos,  BAL_IND_RADIUS_PX*lSize);
    glEnd();
  }
  // return Y axis
  glTranslatef(0, BAL_IND_RANGE_POS*lSize, 0);
}

void DrawTexts(void ) {
  HudConfig* lC = getHudConfig();
  float lSize = lC->size;
  // Draw Text
  SetGLText(); // turn on blending
  //glColor3fv(getHudConfig()->clWindArrow);
  HUDFontProperties *lBF = GetBigFont();
  HUDFontProperties *lSF = GetSmallFont();
  //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);// nastaveni rezimu textury
  float clBlue[] = {0.0f, 0.0f, 1.0f, 0.5f};
  //glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, clBlue);
  // heading
  if (lC->visHeading)
    DrawHUDNumber((int )getHeading(), lBF, -3, 0, lSize/2 - lBF->charHeight, 0);
  // alt
  if (lC->visAlt) {
    DrawHUDNumber((int )convertLength(getAltitude(), LU_FT, lC->altUnits), lBF, 5, -lSize/2, lSize/2 - lBF->charHeight, -1);
    if (lC->showUnits)
      DrawHUDText(getUnitsLabel(lC->altUnits), lSF,
                -lSize/2+getTextWidth(lBF, 5), lSize/2 - lSF->charHeight*2 - 3, 1);
  }

  // radar alt
  if (lC->visRAlt) {
      float rAlt = getRadarAltitude();
      if (rAlt < lC->rngRAltM)
      {
          DrawHUDNumber((int )convertLength(rAlt, LU_M, lC->rAltUnits), lBF, 5, lSize/2, lSize/2 - lBF->charHeight, 1);
          if (getHudConfig()->showUnits)
              DrawHUDText(getUnitsLabel(lC->rAltUnits), lSF,
                          lSize/2, lSize/2 - lSF->charHeight*2 - 3, 1);
      }
  }
  XPLMSetGraphicsState(0, 0, 0, 0, 0, 0, 0); // turn off blending
}

void DrawTorquePerEngine(float *pTrqs, float pConvert, char *pUnits) {
  HudConfig* lC = getHudConfig();
  float lSize = lC->size;
  HUDFontProperties *lSF = GetSmallFont();
  for (int i = 0; i < getNumberOfEngines(); i++) {
    float lTrq = pTrqs[i] * pConvert;
	float lY = lSize/2 - lSF->charHeight - (lSF->charHeight+5) * i;
    DrawHUDNumber((int ) lTrq, lSF, 5, -lSize/2*0.3f, lY, 5);
	if (lC->showUnits)
      DrawHUDText(pUnits, lSF, -lSize/2*0.29f, lY, -1);
  }
}

void DrawCommonTorque(float pTrq, float pConvert, char *pUnits) {
  HudConfig* lC = getHudConfig();
  float lSize = lC->size;
  HUDFontProperties *lBF = GetBigFont();
  // trq
  float lTrq = pTrq * pConvert;
  float lY = lSize/2 - lBF->charHeight;
  DrawHUDNumber((int ) lTrq, lBF, 5, -lSize/2*0.3f, lY, 5);
  if (lC->showUnits)
    DrawHUDText(pUnits, lBF, -lSize/2*0.29f, lY, -1);
}

void DrawTorque(float *pTrqs) {
  SetGLText(); // turn on blending
  float lConvert, lMaxTrq;
  char *lUnits;
  HudConfig* lC = getHudConfig();
  lMaxTrq = (lC->isTrq100Set) ? lC->trq100 : getMaxTorque();
  if (lMaxTrq == 0) {
	  lUnits = "Nm";
	  lConvert = 1.0f;
  } else {
	  lUnits = "%";
	  lConvert = 100.0f / lMaxTrq;
  }
  if (getNumberOfEngines() == 1)
	  DrawCommonTorque(pTrqs[0], lConvert, lUnits);
  else {
	  // check if trq is different per engine
	  float lTrq = pTrqs[0];
	  for (int i = 1; i < getNumberOfEngines(); i++) 
		  if (lTrq != pTrqs[i]) {
			  lTrq = -10000;
			  break;
		  }
	  if (lTrq < -9999)
		  DrawTorquePerEngine(pTrqs, lConvert, lUnits);
	  else
		  DrawCommonTorque(lTrq, lConvert, lUnits);
  }
  XPLMSetGraphicsState(0, 0, 0, 0, 0, 0, 0); // turn off blending
}

