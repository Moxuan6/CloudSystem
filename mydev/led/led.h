#ifndef __LED_H__
#define __LED_H__

#define LED_ON _IOW('L', 0, int)
#define LED_OFF _IOW('L', 1, int)
enum led_stauts {
    OFF,
    ON
};
enum led_no {
    CLED1 = 1,
    CLED2,
    CLED3,
    ELED4,
    ELED5,
    ELED6,
};
#endif