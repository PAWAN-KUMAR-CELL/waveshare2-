#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Screens

enum ScreensEnum {
    _SCREEN_ID_FIRST = 1,
    SCREEN_ID_APPDRAWER1 = 1,
    SCREEN_ID_APPDRAWER2 = 2,
    SCREEN_ID_CAMERA_SCREEN = 3,
    SCREEN_ID_STOPWATCHSCREEN = 4,
    SCREEN_ID_GARDENSCREEN = 5,
    SCREEN_ID_MUSICSCREEN = 6,
    SCREEN_ID_SETTINGSCREEN = 7,
    SCREEN_ID_CLOCKSCREEN = 8,
    _SCREEN_ID_LAST = 8
};

typedef struct _objects_t {
    lv_obj_t *appdrawer1;
    lv_obj_t *appdrawer2;
    lv_obj_t *camera_screen;
    lv_obj_t *stopwatchscreen;
    lv_obj_t *gardenscreen;
    lv_obj_t *musicscreen;
    lv_obj_t *settingscreen;
    lv_obj_t *clockscreen;
    lv_obj_t *grid_area;
    lv_obj_t *okbtn;
    lv_obj_t *nextbtn;
    lv_obj_t *settingbtn;
    lv_obj_t *camerabtn;
    lv_obj_t *stopwatchbtn;
    lv_obj_t *musicbtn;
    lv_obj_t *gardenbtn;
    lv_obj_t *status_bar;
    lv_obj_t *battery;
    lv_obj_t *obj0;
    lv_obj_t *obj1;
    lv_obj_t *obj2;
    lv_obj_t *grid_area_1;
    lv_obj_t *clockbtn;
    lv_obj_t *panda;
    lv_obj_t *musicbtn_1;
    lv_obj_t *gardenbtn_1;
    lv_obj_t *settingbtn_1;
    lv_obj_t *status_bar_2;
    lv_obj_t *battery_2;
    lv_obj_t *obj3;
    lv_obj_t *obj4;
    lv_obj_t *obj5;
    lv_obj_t *ok_1;
    lv_obj_t *next_1;
    lv_obj_t *settingimg;
    lv_obj_t *back_btn;
    lv_obj_t *back;
    lv_obj_t *backbtnnn;
    lv_obj_t *previous;
    lv_obj_t *gardenback;
    lv_obj_t *obj6;
    lv_obj_t *moisturec;
    lv_obj_t *moisturevalue;
    lv_obj_t *obj7;
    lv_obj_t *tempc;
    lv_obj_t *obj8;
    lv_obj_t *obj9;
    lv_obj_t *humidityc;
    lv_obj_t *obj10;
    lv_obj_t *obj11;
    lv_obj_t *gardenok;
    lv_obj_t *gardennext;
    lv_obj_t *status_bar_1;
    lv_obj_t *battery_1;
    lv_obj_t *waternowbtn;
    lv_obj_t *obj12;
    lv_obj_t *obj13;
    lv_obj_t *backkbtn;
    lv_obj_t *back2;
    lv_obj_t *obj14;
    lv_obj_t *obj15;
    lv_obj_t *star1;
    lv_obj_t *star2;
    lv_obj_t *star3;
    lv_obj_t *star4;
    lv_obj_t *star5;
    lv_obj_t *star6;
    lv_obj_t *star7;
    lv_obj_t *star8;
    lv_obj_t *forth_1;
    lv_obj_t *forth2;
    lv_obj_t *forth;
    lv_obj_t *second2;
    lv_obj_t *second;
    lv_obj_t *third;
    lv_obj_t *third3;
    lv_obj_t *third2;
    lv_obj_t *first;
    lv_obj_t *obj16;
    lv_obj_t *obj17;
    lv_obj_t *secondbar;
    lv_obj_t *ap_pm;
    lv_obj_t *obj18;
    lv_obj_t *obj19;
    lv_obj_t *obj20;
    lv_obj_t *obj21;
    lv_obj_t *obj22;
    lv_obj_t *obj23;
    lv_obj_t *clockback;
} objects_t;

extern objects_t objects;

void create_screen_appdrawer1();
void tick_screen_appdrawer1();

void create_screen_appdrawer2();
void tick_screen_appdrawer2();

void create_screen_camera_screen();
void tick_screen_camera_screen();

void create_screen_stopwatchscreen();
void tick_screen_stopwatchscreen();

void create_screen_gardenscreen();
void tick_screen_gardenscreen();

void create_screen_musicscreen();
void tick_screen_musicscreen();

void create_screen_settingscreen();
void tick_screen_settingscreen();

void create_screen_clockscreen();
void tick_screen_clockscreen();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/