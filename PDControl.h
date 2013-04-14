#ifndef _pdCtrl_h_
#define _pdCtrl_h_

#include "kmotor.h"
#include "ktimers.h"
#include "ktypes.h"

#define PDCONTROL_MAX_ROTATION 180

typedef struct PDControl_ PDControl;
struct PDControl_
{
	bool m_enabled;
	uint8_t m_readiness;
	int16_t m_lastAngle;
	int16_t m_targetAngle;
	float m_kp;
	float m_kd;
	uint32_t m_lastMSec;
	Motor* m_motor;
};
void PDControlInit(PDControl* pdc, Motor* motor);
void PDControlSetKp(PDControl* pdc, float kp);
float PDControlGetKp(PDControl* pdc);
void PDControlSetKd(PDControl* pdc, float kd);
float PDControlGetKd(PDControl* pdc);
void PDControlRotate(PDControl* pdc, int16_t angle);
int16_t PDControlGetCurrentAngle(PDControl* pdc);
void PDControlSetEnabled(PDControl* pdc, bool enabled);
bool PDControlIsEnabled(PDControl* pdc);

void taskPDControl(void* arg); // args is PDControl*

#endif // #ifndef _pdCtrl_h_
