#ifndef __SYS_CONFIG_H
#define __SYS_CONFIG_H

// TODO: somehow ensure that only one board is define at a time
// TODO: somehow check for configBOARD_...
//#define configBOARD_V2_0
#define configBOARD_V3_0

// choose one or the other
// TODO: use ifdef to manage both?
//#define configMEMS_2ACT
//#define configMEMS_4ACT
#define configMEMS_4ACT_ORTHO

#ifdef configBOARD_V2_0
	#define configSYS_DDS_AD5932
	#define configSYS_PGA_LM1971_PGA4311
#endif 

#ifdef configBOARD_V3_0
	#define configSYS_DDS_AD9837
	#define configSYS_PGA_CS3308
#endif

// TODO: this could be more specific, how many do we have?
#define configAPPROACH_HORZ
#define configAPPROACH_VERT

// TODO: define these better
#define configMOTOR_PCB
// note that the steppr doesn't exists yet

#define configMOTOR_STEPPER

// Determine controller characteristics
#define featCONTROLLER_PID
#define featCONTROLLER_PI

// General features
#define featCALIBRATION

// TODO: any timing stuff?

#endif
