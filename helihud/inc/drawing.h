#ifndef _drawing_h_
#define _drawing_h_

#define CENTER_BOX_SIZE 0.1f
#define CENTER_BOX_LINE_LENGTH 0.025f
#define NOSE_BOX_SIZE 0.03f
#define NOSE_AREA_SIZE 0.8f
#define CIRCLE_LINES_COUNT 50
#define MOVEMENT_ARROW_RADIUS_PX 0.4f
#define MOVEMENT_ARROW_LEG_SIZE 0.015f
#define LANDING_BARS_RANGE_PX 0.4f
#define LANDING_BARS_SIZE 0.1f
#define VSI_CIRCLE_RANGE_PX 0.4f
#define VSI_RADIUS_PX 0.05f
#define VSI_RANGE_PX 0.4f
#define IAS_RANGE_PX 0.4f
#define GROUND_SPEED_POS 0.3f
#define WIND_ARROW_RADIUS_PX 0.38f
#define WIND_ARROW_SIZE 0.015f
#define WIND_SPEED_POS 0.3f

void TranslateToCenter(void);
void DrawCenterBox(void );
void DrawNoseBox(float pitch, float roll);
void DrawMovementArrow(float trueHeading, float vx, float vy, float vz);
void DrawCircle(int radius);
void DrawVerticalSpeedIndicator(float vv);
void DrawSpeedIndicator(float v);
void DrawWind(float pHeading, float pSpeed, float trueHeading);
void DrawLandingBars(float radarAltitude);
int initGlResources(void );
int destroyGlResources( void );

#endif