#ifndef EEZ_LVGL_UI_VARS_H
#define EEZ_LVGL_UI_VARS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// enum declarations

// Flow global variables

enum FlowGlobalVariables {
    FLOW_GLOBAL_VARIABLE_CURRENT_BTN = 0,
    FLOW_GLOBAL_VARIABLE_OK = 1,
    FLOW_GLOBAL_VARIABLE_HOUR = 2,
    FLOW_GLOBAL_VARIABLE_MINUTES = 3,
    FLOW_GLOBAL_VARIABLE_SECOND = 4,
    FLOW_GLOBAL_VARIABLE_DAY = 5,
    FLOW_GLOBAL_VARIABLE_MOISTURE = 6,
    FLOW_GLOBAL_VARIABLE_TEMP = 7,
    FLOW_GLOBAL_VARIABLE_HUMIDITY = 8,
    FLOW_GLOBAL_VARIABLE_START_PUMP = 9
};

// Native global variables

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/