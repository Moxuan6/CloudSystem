#ifndef __SI7006_H__
#define __SI7006_H__

#define TMP_ADDR 0xe3
#define HUM_ADDR 0xe5

#define GET_TMP _IOR('T',0,int)
#define GET_HUM _IOR('T',1,int)

#endif