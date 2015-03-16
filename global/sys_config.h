#ifndef __SYS_CONFIG_H
#define __SYS_CONFIG_H

// TODO: somehow ensure that only one board is define at a time
// TODO: somehow check for configBOARD_...
#define configBOARD_V2_0
#define configBOARD_V3_0

// choose one or the other
// TODO: use ifdef to manage both?
#define configMEMS_2ACT
#define configMEMS_4ACT
#define configMEMS_4ACT_ORTHO

// choose one or the other
#define configSYS_DDS1234
#define configSYS_DDS4321

// choose one or the other
#define configSYS_PGA1234
#define configSYS_PGA4321

// TODO: this could be more specific, how many do we have?
#define configAPPROACH_HORZ
#define configAPPROACH_VERT

// TODO: define these better
#define configMOTOR_PCB
// note that the stepprt doesn't exists yet

#define configMOTOR_STEPPER

// General features
#define featCALIBRATION

// TODO: any timing stuff?

#endif
