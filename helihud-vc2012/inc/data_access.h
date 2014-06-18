#ifndef _data_access_h_
#define _data_access_h_

#include "XPLMDataAccess.h"

#define SU_KNOTS 1
#define SU_MPS 2
#define SU_KPH 3
#define SU_FPS 4
#define SU_FPM 5

#define SU_MIN 1
#define SU_MAX 5

#define LU_FT 6
#define LU_M 7
#define LU_KM 8
#define LU_NM 9

#define LU_MIN 6
#define LU_MAX 7

// Aicraft dependant values should be loaded only after changing an aircraft
// However some of them are "not ready" yet (call returns 0) so we have to reload
// them every ACF_VALUES_RELOAD_FRAME frame
#define ACF_VALUES_RELOAD_FRAME 100


typedef struct HudConfig {
  char pluginPath[512];
  char acfPath[512];
  int visible;
  int toggleOutside;
  float size;
  char centered;
  int x;
  int y;
  int showUnits;
  char isTrq100Set;
  float trq100;
  // units
  char vsUnits;
  char iasUnits;
  char gsUnits;
  char wsUnits;
  char altUnits;
  char rAltUnits;
  // ranges
  float rngPitchDeg;
  float rngRollDeg;
  float rngMovementArrowMs;
  float rngLandingBarsM;
  float rngVsiFpm;
  float rngVsiBallFpm;
  float rngRAltM;
  float rngIasKt;
  float rngGSKn;
  float rngBalInd;
  float rngYawStr;
  // isVisible switches
  char visVsiBall;
  char visPitchRoll;
  char visMovementArrow;
  char visLandingBars;
  char visVsi;
  char visRAlt;
  char visIas;
  char visGs;
  char visWind;
  char visAlt;
  char visHeading;
  char visBalInd;
  char visYawStr;
  char visTorque;
  // colors
  float clCenterBox[3];
  float clVSI[3];
  float clVSBallLow[3];
  float clVSBallHigh[3];
  float clVSBallMiddle[3];
  float clIAS[3];
  float clMovementForward[3];
  float clMovementBackward[3];
  float clLandingBarsHigh[3];
  float clLandingBarsLow[3];
  float clWindArrow[3];
  float clBalInd[3];
}
HudConfig;

void debugLog(const char *value, ...);
void messageXpl(const char *value);
int findDataRef(const char* name, XPLMDataRef *result);
int initDataRefs(void);
void unregisterData();
float getPitch();
float getRoll();
float getHeading();
float getTrueHeading();
float getVX();
float getVY();
float getVZ();
float getVV();
float getRadarAltitude();
float getAltitude();
float getWindSpeed();
float getWindDirection();
float getIAS();
float getBalance();
float getYawStringAngle();
int getTorque(float *pResult);
float getMaxTorque();
int getNumberOfEngines();
void initAcfValues();

//int getCurrentView();
int getViewIsExternal();


float convertSpeed(float pValue, char pFrom, char pTo);
float convertLength(float pValue, char pFrom, char pTo);
char *getUnitsLabel(char pUnits);
void copyColor(float *pFrom, float *pTo);
void interColor(float *pLow, float *pHigh, float *result, float ratio);

HudConfig* getHudConfig();

HudConfig* initConfig();
void loadConfig(HudConfig* pConfig);
void allignColors(HudConfig* pConfig);
void allignRanges(HudConfig* pConfig);
char isColorSet(float *pColor);

#endif
