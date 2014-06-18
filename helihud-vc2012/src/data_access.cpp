#define XPLM200 1
#define _CRT_SECURE_NO_WARNINGS 1

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "XPLMDataAccess.h"
#include "data_access.h"
#include "XPLMUtilities.h"
#include "XPLMPlugin.h"
#include "XPLMPlanes.h"
#include <stdarg.h>

XPLMDataRef  drPitch;
XPLMDataRef  drRoll;
XPLMDataRef  drTrueHeading;
XPLMDataRef  drHeading;
XPLMDataRef  drVX;
XPLMDataRef  drVY;
XPLMDataRef  drVZ;
XPLMDataRef  drVV;
XPLMDataRef  drRadarAltitude;
XPLMDataRef  drAltitude;
XPLMDataRef  drWindSpeed;
XPLMDataRef  drWindDirection;
XPLMDataRef  drIAS;
//XPLMDataRef  drCurrentView;
XPLMDataRef  drViewIsExternal;
XPLMDataRef  drBalInd;
XPLMDataRef  drYawStr;

// torque
XPLMDataRef  drTrq;
XPLMDataRef  drEnginesNo;
XPLMDataRef  drTrqGreenLo;
XPLMDataRef  drTrqGreenHi;
XPLMDataRef  drTrqYellowLo;
XPLMDataRef  drTrqYellowHi;
XPLMDataRef  drTrqRedLo;
XPLMDataRef  drTrqRedHi;
XPLMDataRef  drMaxTrq;

XPLMDataRef hudVisibleDR = NULL;
XPLMCommandRef toggleHudCommand = NULL;

#define _LOADDEBUG 1

HudConfig lConfig;

char *unitsLables[] = {
                        // WARNING: must be lower case (see config load)
                        "",
                        "kt",   // SU_KNOTS 1
                        "m/s",  // SU_MPS 2
                        "km/h", // SU_KPH 3
                        "ft/s", // SU_FPS 4
                        "ft/m", // SU_FPM 5
                        "ft",   // LU_FT 6
                        "m",    // LU_M 7
                        "km",   // LU_KM 8
                        "nm"    // LU_NM 9
                      };

float clRed[] = { 1.0f, 0.0f, 0.0f };
float clGreen[] = { 0.0f, 1.0f, 0.0f };
float clBlue[] = { 0.0f, 0.0f, 1.0f };
float clPink[] = { 1.0f, 0.2f, 1.0f };
float clNone[] = { 2.0f, 2.0f, 2.0f };

int numberOfEngines = 1;
float tqGreenLo;
float tqGreenHi;
float tqYellowLo;
float tqYellowHi;
float tqRedLo;
float tqRedHi;
float tqMax;

int     GetHudVisibleCB(void* inRefcon);
void SetHudVisibleCB(void* inRefcon, int inValue);
int    toggleHudCommandHandler(XPLMCommandRef       inCommand,
                               XPLMCommandPhase     inPhase,
                               void *               inRefcon);

void debugLog(const char *fmt, ...) {
	char buffer[1024];
	// if I ever send debug string longer than 1024 bytes - "HELIHUD: ",
	// I will never find this error why application crashes :-)
	va_list ap;
    va_start(ap,fmt);
	strcpy(buffer, "HELIHUD:  ");
    vsprintf(buffer+9, fmt, ap);
    va_end(ap);
	XPLMDebugString(buffer);
}

void messageXpl(const char *value) {
  XPLMSpeakString(value);
}

int findDataRef(const char* name, XPLMDataRef *result) {
  *result = XPLMFindDataRef(name);
  if (*result == NULL)
  {
    debugLog("Error finding XPL variable ");
    debugLog(name);
    debugLog("\n");
    return -1;
  }
  return 0;
}

int registerDataRefs() {
  //  Create our custom integer dataref
  hudVisibleDR = XPLMRegisterDataAccessor("Helihud/Visible",
                                          xplmType_Int,                                  // The types we support
                                          1,                                             // Writable
                                          GetHudVisibleCB, SetHudVisibleCB,              // Integer accessors
                                          NULL, NULL,                                    // Float accessors
                                          NULL, NULL,                                    // Doubles accessors
                                          NULL, NULL,                                    // Int array accessors
                                          NULL, NULL,                                    // Float array accessors
                                          NULL, NULL,                                    // Raw data accessors
                                          NULL, NULL);                                   // Refcons not used

  // Find and intialize our dataref
  hudVisibleDR = XPLMFindDataRef ("Helihud/Visible");
  if (hudVisibleDR == NULL)
    return -1;
  else
  {
    XPLMSetDatai(hudVisibleDR, 0);
    return 0;
  }
}

int registerCommands() {
  toggleHudCommand = XPLMCreateCommand("Helihud/Toggle", "View/hide helihud");
  XPLMRegisterCommandHandler(toggleHudCommand,           // in Command name
                             toggleHudCommandHandler,    // in Handler
                             1,                          // Receive input before plugin windows.
                             (void *) 0);                // inRefcon.
  return 0;
}

HudConfig* initConfig() {
  char lFileName[256];
  // find out plugin path
  XPLMPluginID lMyID = XPLMGetMyID();
  XPLMGetPluginInfo(lMyID, NULL, lConfig.pluginPath, NULL, NULL);
  char *lFilePart = XPLMExtractFileAndPath(lConfig.pluginPath);
  const char *lSep = XPLMGetDirectorySeparator();
  if (lFilePart != NULL)
  {
    *lFilePart = 0;
    strcat(lConfig.pluginPath, lSep);
  }
  // find out aircraft path
  XPLMGetNthAircraftModel(0, lFileName, lConfig.acfPath);
  lFilePart = XPLMExtractFileAndPath(lConfig.acfPath);
  if (lFilePart != NULL)
  {
    *lFilePart = 0;
    strcat(lConfig.acfPath, lSep);
  }
  lConfig.visible = 1;
  lConfig.toggleOutside = 1;
  lConfig.size = 500.0f;
  lConfig.centered = 1;
  lConfig.x = -1;
  lConfig.y = -1;
  lConfig.vsUnits = SU_FPM;
  lConfig.iasUnits = SU_KNOTS;
  lConfig.gsUnits = SU_KPH;
  lConfig.wsUnits = SU_KNOTS;
  lConfig.altUnits = LU_FT;
  lConfig.rAltUnits = LU_FT;
  lConfig.showUnits = 1;
  lConfig.isTrq100Set = 0;
  lConfig.trq100 = 0;
  // set default ranges
  lConfig.rngPitchDeg = -1;
  lConfig.rngRollDeg = -1;
  lConfig.rngMovementArrowMs = -1;
  lConfig.rngLandingBarsM = -1;
  lConfig.rngVsiFpm = -1;
  lConfig.rngVsiBallFpm = -1;
  lConfig.rngRAltM = -1;
  lConfig.rngIasKt = -1;
  lConfig.rngGSKn = -1;
  lConfig.rngBalInd = 20; //deg
  lConfig.rngYawStr = 30; //deg
  // set default colors
  copyColor(clNone, lConfig.clCenterBox);
  copyColor(clNone, lConfig.clVSI);
  copyColor(clNone, lConfig.clVSBallLow);
  copyColor(clNone, lConfig.clVSBallHigh);
  copyColor(clNone, lConfig.clVSBallMiddle);
  copyColor(clNone, lConfig.clIAS);
  copyColor(clNone, lConfig.clMovementForward);
  copyColor(clNone, lConfig.clMovementBackward);
  copyColor(clNone, lConfig.clLandingBarsHigh);
  copyColor(clNone, lConfig.clLandingBarsLow);
  copyColor(clNone, lConfig.clWindArrow);
  copyColor(clNone, lConfig.clBalInd);
  // set default everything visible
  lConfig.visVsiBall = 1;
  lConfig.visPitchRoll = 1;
  lConfig.visMovementArrow = 1;
  lConfig.visLandingBars = 1;
  lConfig.visVsi = 1;
  lConfig.visRAlt = 1;
  lConfig.visIas = 1;
  lConfig.visGs = 1;
  lConfig.visWind = 1;
  lConfig.visAlt = 1;
  lConfig.visHeading = 1;
  lConfig.visBalInd = 1;
  lConfig.visYawStr = 1;
  lConfig.visTorque = 1;
  // load from file
  loadConfig(&lConfig);
  // replace clNone by defaults
  allignColors(&lConfig);
  // replace -1 by defaults
  allignRanges(&lConfig);
  return &lConfig;
}

char *trimWhiteSpace(char *str)
{
  char *end;
  // Trim leading space
  while(isspace(*str))
    str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace(*end))
    end--;

  // Write new null terminator
  *(end+1) = 0;

  return str;
}

void loadFloat(float *pRes, const char *pKey, const char *pValue, float pMin, float pMax) {
  char logMessage[255];
  float lRes = atof(pValue);
  if (lRes >= pMin && lRes <= pMax)
  {
    *pRes = lRes;
#ifdef LOADDEBUG

    sprintf(logMessage, "HELIHUD DEBUG: Config value %f for key %s set.\n",
            lRes, pKey);
    debugLog(logMessage);
#endif

  }
  else
  {
    sprintf(logMessage, "HELIHUD: Config value %f for key %s is out of allowed range (%f - %f).\n",
            lRes, pKey, pMin, pMax);
    debugLog(logMessage);
  }
}

void loadInt(int *pRes, const char *pKey, const char *pValue, int pMin, int pMax) {
  char logMessage[255];
  int lRes = atoi(pValue);
  if (lRes >= pMin && lRes <= pMax)
  {
    *pRes = lRes;
#ifdef LOADDEBUG

    sprintf(logMessage, "HELIHUD DEBUG: Config value %d for key %s set.\n",
            lRes, pKey);
    debugLog(logMessage);
#endif

  }
  else
  {
    sprintf(logMessage, "HELIHUD: Config value %f for key %s is out of allowed range (%d - %d).\n",
            lRes, pKey, pMin, pMax);
    debugLog(logMessage);
  }
}

void loadChar(char *pRes, const char *pKey, const char *pValue, char pMin, char pMax) {
    int lRes;
    loadInt(&lRes, pKey, pValue, pMin, pMax);
    *pRes = (char) lRes;
}

void loadUnit(char *pRes, const char *pKey, char *pValue, int pMin, int pMax) {
  char logMessage[255];
  // convert value to lower case
  char *lP;
  lP = pValue;
  while (*lP)
    *lP++ = tolower(*lP);
  for (int i = pMin; i <= pMax; i++)
  {
    if (strcmp(pValue, unitsLables[i]) == 0)
    {
      *pRes = i;
#ifdef LOADDEBUG

      sprintf(logMessage, "HELIHUD DEBUG: Config value %d for key %s set.\n",
              *pRes, pKey);
      debugLog(logMessage);
#endif

      return;
    }
  }
  sprintf(logMessage, "HELIHUD: Unit value %s for key %s not recognized.\n",
          pValue, pKey);
  debugLog(logMessage);
}

float parseColor(char *pIn) {
  float lRes = 0;
  if (*pIn >= 'A' && *pIn <= 'F')
    lRes = (*pIn - 'A' + 10) * 16;
  else
    lRes = (*pIn - '0') * 16;
  pIn++;
  if (*pIn >= 'A' && *pIn <= 'F')
    lRes += (*pIn - 'A' + 10);
  else
    lRes += (*pIn - '0');
  lRes /= 255.0f;
  return(lRes);
}

void loadColor(float *pRes, const char *pKey, char *pValue) {
  char logMessage[255];
  // convert value to upper case
  char *lP;
  char lCount = 0;
  lP = pValue;
  while (*lP && lCount < 6)
  {
    if (*lP >= 'a' && *lP <= 'f')
      *lP = *lP - 'a' + 'A';
    if ((*lP >= 'A' && *lP <= 'F') || (*lP >= '0' && *lP <= '9'))
    {
      lP++;
      lCount++;
    }
    else
    {
      lCount = 10; // error
    }
  }
  if (lCount != 6 || *lP != 0)
  {
    sprintf(logMessage, "HELIHUD: Color value %s for key %s not recognized. Provide color in HTML format.\n",
            pValue, pKey);
    debugLog(logMessage);
    return;
  }
  else
  {
    pRes[0] = parseColor(pValue);
    pRes[1] = parseColor(pValue+2);
    pRes[2] = parseColor(pValue+4);
#ifdef LOADDEBUG

    sprintf(logMessage, "HELIHUD DEBUG: Color %s for key %s parsed as %f, %f, %f.\n",
            pValue, pKey, pRes[0], pRes[1], pRes[2]);
    debugLog(logMessage);
#endif

  }
}

void loadConfigValue(HudConfig* pConfig, char *pKey, char *pValue) {
  char *lP;
  lP = pKey;
  while (*lP)
    *lP++ = toupper(*lP);
  if (strcmp(pKey, "SIZE") == 0)
    loadFloat(&pConfig->size, pKey, pValue, 10, 1000);
  else if (strcmp(pKey, "POSX") == 0)
    loadInt(&pConfig->x, pKey, pValue, 0, 5000);
  else if (strcmp(pKey, "POSY") == 0)
    loadInt(&pConfig->y, pKey, pValue, 0, 5000);
  else if (strcmp(pKey, "SHOW_UNITS") == 0)
    loadInt(&pConfig->showUnits, pKey, pValue, 0, 1);
  else if (strcmp(pKey, "START_VISIBLE") == 0)
    loadInt(&pConfig->visible, pKey, pValue, 0, 1);
  else if (strcmp(pKey, "HIDE_IN_EXTERNAL_VIEW") == 0)
    loadInt(&pConfig->toggleOutside, pKey, pValue, 0, 1);
  else if (strcmp(pKey, "TORQUE_100") == 0) {
	  loadFloat(&pConfig->trq100, pKey, pValue, 0, 100000);
	  pConfig->isTrq100Set = 1;
  }
  // speed units
  else if (strcmp(pKey, "VS_UNITS") == 0)
    loadUnit(&pConfig->vsUnits, pKey, pValue, SU_MIN, SU_MAX);
  else if (strcmp(pKey, "IAS_UNITS") == 0)
    loadUnit(&pConfig->iasUnits, pKey, pValue, SU_MIN, SU_MAX);
  else if (strcmp(pKey, "GS_UNITS") == 0)
    loadUnit(&pConfig->gsUnits, pKey, pValue, SU_MIN, SU_MAX);
  else if (strcmp(pKey, "WS_UNITS") == 0)
    loadUnit(&pConfig->wsUnits, pKey, pValue, SU_MIN, SU_MAX);
  // length units
  else if (strcmp(pKey, "ALT_UNITS") == 0)
    loadUnit(&pConfig->altUnits, pKey, pValue, LU_MIN, LU_MAX);
  else if (strcmp(pKey, "RADAR_ALT_UNITS") == 0)
    loadUnit(&pConfig->rAltUnits, pKey, pValue, LU_MIN, LU_MAX);
  // ranges
  else if (strcmp(pKey, "PITCH_RANGE") == 0)
    loadFloat(&pConfig->rngPitchDeg, pKey, pValue, 1, 100);
  else if (strcmp(pKey, "ROLL_RANGE") == 0)
    loadFloat(&pConfig->rngRollDeg, pKey, pValue, 1, 100);
  else if (strcmp(pKey, "MOVEMENT_ARROW_RANGE") == 0)
    loadFloat(&pConfig->rngMovementArrowMs, pKey, pValue, 1, 10000);
  else if (strcmp(pKey, "LANDING_BARS_RANGE") == 0)
    loadFloat(&pConfig->rngLandingBarsM, pKey, pValue, 1, 10000);
  else if (strcmp(pKey, "VSI_RANGE") == 0)
    loadFloat(&pConfig->rngVsiFpm, pKey, pValue, 1, 10000);
  else if (strcmp(pKey, "VS_BALL_RANGE") == 0)
    loadFloat(&pConfig->rngVsiBallFpm, pKey, pValue, 1, 10000);
  else if (strcmp(pKey, "RADAR_ALT_RANGE") == 0)
    loadFloat(&pConfig->rngRAltM, pKey, pValue, 1, 10000);
  else if (strcmp(pKey, "IAS_RANGE") == 0)
    loadFloat(&pConfig->rngIasKt, pKey, pValue, 1, 10000);
  else if (strcmp(pKey, "GS_RANGE") == 0)
    loadFloat(&pConfig->rngGSKn, pKey, pValue, 1, 10000);
  else if (strcmp(pKey, "BAL_IND_RANGE") == 0)
    loadFloat(&pConfig->rngBalInd, pKey, pValue, 1, 100);
  else if (strcmp(pKey, "YAW_STR_RANGE") == 0)
    loadFloat(&pConfig->rngYawStr, pKey, pValue, 1, 100);
  // colors
  else if (strcmp(pKey, "VSI_COLOR") == 0)
    loadColor(pConfig->clVSI, pKey, pValue);
  else if (strcmp(pKey, "ROLL_PITCH_COLOR") == 0)
    loadColor(pConfig->clCenterBox, pKey, pValue);
  else if (strcmp(pKey, "VS_BALL_LOW_COLOR") == 0)
    loadColor(pConfig->clVSBallLow, pKey, pValue);
  else if (strcmp(pKey, "VS_BALL_MIDDLE_COLOR") == 0)
    loadColor(pConfig->clVSBallMiddle, pKey, pValue);
  else if (strcmp(pKey, "VS_BALL_HIGH_COLOR") == 0)
    loadColor(pConfig->clVSBallHigh, pKey, pValue);
  else if (strcmp(pKey, "IAS_COLOR") == 0)
    loadColor(pConfig->clIAS, pKey, pValue);
  else if (strcmp(pKey, "MOVEMENT_ARROW_FWD_COLOR") == 0)
    loadColor(pConfig->clMovementForward, pKey, pValue);
  else if (strcmp(pKey, "MOVEMENT_ARROW_BCK_COLOR") == 0)
    loadColor(pConfig->clMovementBackward, pKey, pValue);
  else if (strcmp(pKey, "LANDING_BARS_HIGH_COLOR") == 0)
    loadColor(pConfig->clLandingBarsHigh, pKey, pValue);
  else if (strcmp(pKey, "LANDING_BARS_LOW_COLOR") == 0)
    loadColor(pConfig->clLandingBarsLow, pKey, pValue);
  else if (strcmp(pKey, "WIND_COLOR") == 0)
    loadColor(pConfig->clWindArrow, pKey, pValue);
  else if (strcmp(pKey, "BAL_IND_COLOR") == 0)
    loadColor(pConfig->clBalInd, pKey, pValue);
// visible flags
  else if (strcmp(pKey, "VS_BALL_VISIBLE") == 0)
    loadChar(&pConfig->visVsiBall, pKey, pValue, 0, 1);
  else if (strcmp(pKey, "PITCH_ROLL_VISIBLE") == 0)
    loadChar(&pConfig->visPitchRoll, pKey, pValue, 0, 1);
  else if (strcmp(pKey, "MOVEMENT_ARROW_VISIBLE") == 0)
    loadChar(&pConfig->visMovementArrow, pKey, pValue, 0, 1);
  else if (strcmp(pKey, "LANDING_BARS_VISIBLE") == 0)
    loadChar(&pConfig->visLandingBars, pKey, pValue, 0, 1);
  else if (strcmp(pKey, "VSI_VISIBLE") == 0)
    loadChar(&pConfig->visVsi, pKey, pValue, 0, 1);
  else if (strcmp(pKey, "RADAR_ALT_VISIBLE") == 0)
    loadChar(&pConfig->visRAlt, pKey, pValue, 0, 1);
  else if (strcmp(pKey, "IAS_VISIBLE") == 0)
    loadChar(&pConfig->visIas, pKey, pValue, 0, 1);
  else if (strcmp(pKey, "GS_VISIBLE") == 0)
    loadChar(&pConfig->visGs, pKey, pValue, 0, 1);
  else if (strcmp(pKey, "WIND_VISIBLE") == 0)
    loadChar(&pConfig->visWind, pKey, pValue, 0, 1);
  else if (strcmp(pKey, "ALT_VISIBLE") == 0)
    loadChar(&pConfig->visAlt, pKey, pValue, 0, 1);
  else if (strcmp(pKey, "HEADING_VISIBLE") == 0)
    loadChar(&pConfig->visHeading, pKey, pValue, 0, 1);
  else if (strcmp(pKey, "BAL_IND_VISIBLE") == 0)
    loadChar(&pConfig->visBalInd, pKey, pValue, 0, 1);
  else if (strcmp(pKey, "YAW_STR_VISIBLE") == 0)
    loadChar(&pConfig->visYawStr, pKey, pValue, 0, 1);
  else if (strcmp(pKey, "TORQUE_VISIBLE") == 0)
      loadChar(&pConfig->visTorque, pKey, pValue, 0, 1);
  //debugLog(pKey); debugLog(pValue);
}

void loadConfig(HudConfig* pConfig) {
  char lConfigFileName[512];
  FILE* lFile = NULL;
  strcpy(lConfigFileName, getHudConfig()->acfPath);
  strcat(lConfigFileName, "helihud.ini");
  debugLog("Trying config file %s.\n", lConfigFileName);
  if (!(lFile = fopen(lConfigFileName, "r"))) {
    strcpy(lConfigFileName, getHudConfig()->pluginPath);
    strcat(lConfigFileName, "helihud.ini");
	lFile = fopen(lConfigFileName, "r");
  }
  if (lFile)
  {
    debugLog("Reading config file %s.\n", lConfigFileName);
    char lLine[1024];
    char *lL;
    char *lK;
    char *lV;
    while (!feof(lFile))
    {
      if (!fgets(lLine, 1024, lFile))
        break;
      // trim the line
      lL = trimWhiteSpace(lLine);
      // is it comment?
      if (lL[0] == ';')
        continue;
      // parse key + value
      char *lVal = strstr(lL, "=");
      if (lVal != NULL)
      {
        *lVal = 0;
        lK = trimWhiteSpace(lL);
        lV = trimWhiteSpace(lVal+1);
        loadConfigValue(pConfig, lK, lV);
      }
    }
    fclose(lFile);
    if (pConfig->x >= 0 && pConfig->y >= 0)
    {
      pConfig->centered = 0;
    }
    else
    {
      pConfig->x = -1;
      pConfig->y = -1;
    }
  }
  else
  {
    debugLog("Config file %s does not exist, using default values.\n", lConfigFileName);
  }
}

void allignColors(HudConfig* pConfig) {
  if (!isColorSet(pConfig->clCenterBox))
    copyColor(clPink, pConfig->clCenterBox);
  if (!isColorSet(pConfig->clVSI))
    copyColor(clGreen, pConfig->clVSI);
  if (!isColorSet(pConfig->clVSBallLow))
    copyColor(clRed, pConfig->clVSBallLow);
  if (!isColorSet(pConfig->clVSBallHigh))
    copyColor(clGreen, pConfig->clVSBallHigh);
  if (!isColorSet(pConfig->clVSBallMiddle))
    interColor(pConfig->clVSBallLow, pConfig->clVSBallHigh, pConfig->clVSBallMiddle, 0.5f);
  if (!isColorSet(pConfig->clIAS))
    copyColor(clGreen, pConfig->clIAS);
  if (!isColorSet(pConfig->clWindArrow))
    copyColor(clBlue, pConfig->clWindArrow);
  if (!isColorSet(pConfig->clBalInd))
    copyColor(clPink, pConfig->clBalInd);

  if (!isColorSet(pConfig->clMovementBackward) && !isColorSet(pConfig->clMovementForward))
  {
    copyColor(clGreen, lConfig.clMovementForward);
    copyColor(clRed, lConfig.clMovementBackward);
  }
  else if (isColorSet(pConfig->clMovementBackward) && !isColorSet(pConfig->clMovementForward))
  {
    copyColor(pConfig->clMovementBackward, lConfig.clMovementForward);
  }
  else if (!isColorSet(pConfig->clMovementBackward) && isColorSet(pConfig->clMovementForward))
  {
    copyColor(pConfig->clMovementForward, lConfig.clMovementBackward);
  }

  if (!isColorSet(pConfig->clLandingBarsLow) && !isColorSet(pConfig->clLandingBarsHigh))
  {
    copyColor(clGreen, lConfig.clLandingBarsHigh);
    copyColor(clRed, lConfig.clLandingBarsLow);
  }
  else if (isColorSet(pConfig->clLandingBarsLow) && !isColorSet(pConfig->clLandingBarsHigh))
  {
    copyColor(pConfig->clLandingBarsLow, lConfig.clLandingBarsHigh);
  }
  else if (!isColorSet(pConfig->clLandingBarsLow) && isColorSet(pConfig->clLandingBarsHigh))
  {
    copyColor(pConfig->clLandingBarsHigh, lConfig.clLandingBarsLow);
  }
}


HudConfig* getHudConfig() {
  return &lConfig;
}

int initDataRefs() {
  int lTmp;
  /* Also look up our data refs. */
  lTmp = 0;
  lTmp += findDataRef("sim/flightmodel/position/theta", &drPitch);
  lTmp += findDataRef("sim/flightmodel/position/phi", &drRoll);
  lTmp += findDataRef("sim/flightmodel/position/magpsi", &drHeading);
  lTmp += findDataRef("sim/flightmodel/position/psi", &drTrueHeading);
  lTmp += findDataRef("sim/flightmodel/position/local_vx", &drVX);
  lTmp += findDataRef("sim/flightmodel/position/local_vy", &drVY);
  lTmp += findDataRef("sim/flightmodel/position/local_vz", &drVZ);
  lTmp += findDataRef("sim/flightmodel/position/vh_ind_fpm", &drVV);
  lTmp += findDataRef("sim/flightmodel/position/y_agl", &drRadarAltitude);
  lTmp += findDataRef("sim/flightmodel/misc/h_ind", &drAltitude);
  lTmp += findDataRef("sim/cockpit2/gauges/indicators/wind_speed_kts", &drWindSpeed);
  lTmp += findDataRef("sim/cockpit2/gauges/indicators/wind_heading_deg_mag", &drWindDirection);
  //lTmp += findDataRef("sim/weather/wind_direction_degt[0]", &drWindDirection);
  lTmp += findDataRef("sim/flightmodel/position/indicated_airspeed", &drIAS);
  //lTmp += findDataRef("sim/flightmodel/position/magnetic_variation", &drMagVar);
  //lTmp += findDataRef("sim/graphics/view/view_type", &drCurrentView);
  lTmp += findDataRef("sim/graphics/view/view_is_external", &drViewIsExternal);
  lTmp += findDataRef("sim/cockpit2/gauges/indicators/slip_deg", &drBalInd);
  lTmp += findDataRef("sim/flightmodel2/misc/yaw_string_angle", &drYawStr);
  lTmp += findDataRef("sim/flightmodel/engine/ENGN_TRQ", &drTrq);  //NewtonMeters
  lTmp += findDataRef("sim/aircraft/engine/acf_num_engines", &drEnginesNo);
  lTmp += findDataRef("sim/aircraft/limits/green_lo_TRQ", &drTrqGreenLo); // ft-lbs
  lTmp += findDataRef("sim/aircraft/limits/green_hi_TRQ", &drTrqGreenHi);
  lTmp += findDataRef("sim/aircraft/limits/yellow_lo_TRQ", &drTrqYellowLo);
  lTmp += findDataRef("sim/aircraft/limits/yellow_hi_TRQ", &drTrqYellowHi);
  lTmp += findDataRef("sim/aircraft/limits/red_lo_TRQ", &drTrqRedLo);
  lTmp += findDataRef("sim/aircraft/limits/red_hi_TRQ", &drTrqRedHi);
  lTmp += findDataRef("sim/aircraft/controls/acf_trq_max_eng", &drMaxTrq);

  lTmp += registerDataRefs();
  lTmp += registerCommands();

  return lTmp;
}

void unregisterData() {
  XPLMUnregisterDataAccessor(hudVisibleDR);
  XPLMUnregisterCommandHandler(toggleHudCommand, toggleHudCommandHandler, 0, 0);
}

void initAcfValues() {
  numberOfEngines = XPLMGetDatai(drEnginesNo);
  tqGreenLo = XPLMGetDataf(drTrqGreenLo);
  tqGreenHi = XPLMGetDataf(drTrqGreenHi);
  tqYellowLo = XPLMGetDataf(drTrqYellowLo);
  tqYellowHi = XPLMGetDataf(drTrqYellowHi);
  tqRedLo = XPLMGetDataf(drTrqRedLo);
  tqRedHi = XPLMGetDataf(drTrqRedHi);
  tqMax = XPLMGetDataf(drMaxTrq);
  /*
  debugLog("Aircraft init:\nNumber of engines: %d\n", numberOfEngines);
  debugLog("Max torque: %f\n", tqMax);
  debugLog("GL: %f\n", tqGreenLo);
  debugLog("GH: %f\n", tqGreenHi);
  debugLog("YL: %f\n", tqYellowLo);
  debugLog("YH: %f\n", tqYellowHi);
  debugLog("RL: %f\n", tqRedLo);
  debugLog("RH: %f\n", tqRedHi);
   */
}

float getMaxTorque() {
	return tqMax;
}
int getNumberOfEngines() {
	return numberOfEngines;
}
int getTorque(float *pResult) {
  return XPLMGetDatavf(drTrq, pResult, 0, numberOfEngines);
}
float getPitch() {
  return XPLMGetDataf(drPitch);
}
float getRoll() {
  return XPLMGetDataf(drRoll);
}
float getHeading() {
  return XPLMGetDataf(drHeading);
}
float getTrueHeading() {
  return XPLMGetDataf(drTrueHeading);
}
float getVX() {
  return XPLMGetDataf(drVX);
}
float getVY() {
  return XPLMGetDataf(drVY);
}
float getVZ() {
  return XPLMGetDataf(drVZ);
}
float getVV() {
  return XPLMGetDataf(drVV);
}
float getRadarAltitude() {
  return XPLMGetDataf(drRadarAltitude);
}
float getAltitude() {
  return XPLMGetDataf(drAltitude);
}
float getWindSpeed() {
  return XPLMGetDataf(drWindSpeed);
}
float getWindDirection() {
  return XPLMGetDataf(drWindDirection);
}
float getIAS() {
  return XPLMGetDataf(drIAS);
}
float getBalance() {
  return XPLMGetDataf(drBalInd);
}
float getYawStringAngle() {
  return XPLMGetDataf(drYawStr);
}


//int getCurrentView() {
//  return XPLMGetDatai(drCurrentView);
//}

int getViewIsExternal() {
  return XPLMGetDatai(drViewIsExternal);
}

int     GetHudVisibleCB(void* inRefcon)
{
  return lConfig.visible;
}

void SetHudVisibleCB(void* inRefcon, int inValue)
{
  if (inValue)
    lConfig.visible = 1;
  else
    lConfig.visible = 0;
}

int    toggleHudCommandHandler(XPLMCommandRef       inCommand,
                               XPLMCommandPhase     inPhase,
                               void *               inRefcon)
{
  //  If inPhase == 0 the command is executed once on button down.
  if (inPhase == 0)
  {
	  if (lConfig.toggleOutside && getViewIsExternal() && lConfig.visible < 2)
		  lConfig.visible = 2;
	  else
		  if (lConfig.visible)
			  lConfig.visible = 0;
		  else
			  lConfig.visible = 1;
  //char buffer[255];
  //sprintf(buffer, "Visible set to %d.\n", lConfig.visible);
  //debugLog(buffer);
  }
  // Return 1 to pass the command to plugin windows and X-Plane.
  // Returning 0 disables further processing by X-Plane.
  return 0;
}

float convertSpeed(float pValue, char pFrom, char pTo) {
  if (pFrom == pTo)
    return pValue;
  float lResult = pValue;
  // convert to m/s first
  switch (pFrom)
  {
  case SU_KNOTS:
    lResult *= (1852.0f/3600.0f);
    break;
  case SU_KPH:
    lResult *= (1000.0f/3600.0f);
    break;
  case SU_FPS:
    lResult *= (0.3048f);
    break;
  case SU_FPM:
    lResult *= (0.3048f/60.0f);
    break;
  }
  // now convert to target
  switch (pTo)
  {
  case SU_KNOTS:
    lResult *= (3600/1852);
    break;
  case SU_KPH:
    lResult *= (3.6f);
    break;
  case SU_FPS:
    lResult *= (1.0f/0.3048f);
    break;
  case SU_FPM:
    lResult *= (60.0f/0.3048f);
    break;
  }
  return lResult;
}

float convertLength(float pValue, char pFrom, char pTo) {
  if (pFrom == pTo)
    return pValue;
  float lResult = pValue;
  // convert to m first
  switch (pFrom)
  {
  case LU_FT:
    lResult *= (0.3048f);
    break;
  case LU_KM:
    lResult *= (1000.0f);
    break;
  case LU_NM:
    lResult *= (1852.0f);
    break;
  }
  // now convert to target
  switch (pTo)
  {
  case LU_FT:
    lResult /= (0.3048f);
    break;
  case LU_KM:
    lResult /= (1000.0f);
    break;
  case LU_NM:
    lResult /= (1852.0f);
    break;
  }
  return lResult;
}

char *getUnitsLabel(char pUnits) {
  return unitsLables[pUnits];
}

void copyColor(float *pFrom, float *pTo) {
  pTo[0] = pFrom[0];
  pTo[1] = pFrom[1];
  pTo[2] = pFrom[2];
}

void interColor(float *pLow, float *pHigh, float *result, float ratio) {
  for (int i = 0; i < 3; i++)
  {
    float delta = pHigh[i] - pLow[i];
    result[i] = pLow[i] + delta * ratio;
  }
}

char isColorSet(float *pColor) {
  return (pColor[0] == 2.0f || pColor[1] == 2.0f || pColor[2] == 2.0f) ? 0 : 1;
}

void allignRanges(HudConfig* pConfig) {
  if (pConfig->rngPitchDeg == -1)
    pConfig->rngPitchDeg = 10;

  if (pConfig->rngRollDeg == -1)
    pConfig->rngRollDeg = 10;

  if (pConfig->rngMovementArrowMs == -1)
    pConfig->rngMovementArrowMs = 15.433333333333f;  // 30kn
  else
    pConfig->rngMovementArrowMs = convertSpeed(pConfig->rngMovementArrowMs, pConfig->iasUnits, SU_MPS);

  if (pConfig->rngLandingBarsM == -1)
    pConfig->rngLandingBarsM = 30.48f; // 100ft
  else
    pConfig->rngLandingBarsM = convertLength(pConfig->rngLandingBarsM, pConfig->rAltUnits, LU_M);

  if (pConfig->rngVsiFpm == -1)
    pConfig->rngVsiFpm = 1000;
  else
    pConfig->rngVsiFpm = convertSpeed(pConfig->rngVsiFpm, pConfig->vsUnits, SU_FPM);

  if (pConfig->rngVsiBallFpm == -1)
    pConfig->rngVsiBallFpm = 100;
  else
    pConfig->rngVsiBallFpm = convertSpeed(pConfig->rngVsiBallFpm, pConfig->vsUnits, SU_FPM);

  if (pConfig->rngRAltM == -1)
    pConfig->rngRAltM = 300.48f;  //1000ft
  else
    pConfig->rngRAltM = convertLength(pConfig->rngRAltM, pConfig->rAltUnits, LU_M);

  if (pConfig->rngIasKt == -1)
    pConfig->rngIasKt = 300;
  else
    pConfig->rngIasKt = convertSpeed(pConfig->rngIasKt, pConfig->iasUnits, SU_KNOTS);

  if (pConfig->rngGSKn == -1)
    pConfig->rngGSKn = 30;
  else
    pConfig->rngGSKn = convertSpeed(pConfig->rngGSKn, pConfig->gsUnits, SU_KNOTS);
}

