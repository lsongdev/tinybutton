#ifndef TINYBUTTON_H
#define TINYBUTTON_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TINYBUTTON_DEFAULT_DEBOUNCE_MS 30u
#define TINYBUTTON_DEFAULT_CLICK_MS 300u
#define TINYBUTTON_DEFAULT_LONG_PRESS_MS 800u
#define TINYBUTTON_DEFAULT_REPEAT_MS 0u

typedef enum {
    TINYBUTTON_EVENT_PRESS = 0,
    TINYBUTTON_EVENT_RELEASE,
    TINYBUTTON_EVENT_CLICK,
    TINYBUTTON_EVENT_LONG_PRESS,
    TINYBUTTON_EVENT_LONG_REPEAT,
    TINYBUTTON_EVENT_LONG_RELEASE,
} tinybutton_event_t;

typedef struct {
    uint32_t debounce_ms;
    uint32_t click_ms;
    uint32_t long_press_ms;
    uint32_t repeat_ms;
} tinybutton_config_t;

struct tinybutton;
typedef struct tinybutton tinybutton_t;

typedef void (*tinybutton_callback_t)(
    tinybutton_t *button,
    tinybutton_event_t event,
    uint8_t clicks,
    void *user);

struct tinybutton {
    tinybutton_config_t config;
    tinybutton_callback_t callback;
    void *user;

    uint32_t raw_changed_at;
    uint32_t pressed_at;
    uint32_t released_at;
    uint32_t repeated_at;

    uint8_t clicks;
    bool initialized;
    bool raw_pressed;
    bool pressed;
    bool long_pressed;
};

tinybutton_config_t tinybutton_default_config(void);

void tinybutton_init(tinybutton_t *button, const tinybutton_config_t *config);
void tinybutton_reset(tinybutton_t *button);
void tinybutton_configure(tinybutton_t *button, const tinybutton_config_t *config);
void tinybutton_set_callback(
    tinybutton_t *button,
    tinybutton_callback_t callback,
    void *user);

void tinybutton_update(tinybutton_t *button, bool pressed, uint32_t now_ms);

bool tinybutton_is_pressed(const tinybutton_t *button);
bool tinybutton_is_long_pressed(const tinybutton_t *button);

#ifdef __cplusplus
}
#endif

#endif
