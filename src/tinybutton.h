#ifndef TINYBUTTON_H
#define TINYBUTTON_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TB_DEFAULT_DEBOUNCE_MS 30u
#define TB_DEFAULT_CLICK_MS 300u
#define TB_DEFAULT_LONG_PRESS_MS 800u
#define TB_DEFAULT_REPEAT_MS 0u

typedef enum {
    TB_EVENT_PRESS = 0,
    TB_EVENT_RELEASE,
    TB_EVENT_CLICK,
    TB_EVENT_LONG_PRESS,
    TB_EVENT_LONG_REPEAT,
    TB_EVENT_LONG_RELEASE,
} tb_event;

typedef struct {
    uint32_t debounce_ms;
    uint32_t click_ms;
    uint32_t long_press_ms;
    uint32_t repeat_ms;
} tb_config;

struct tb_button;
typedef struct tb_button tb_button;

typedef void (*tb_button_cb)(
    tb_button *button,
    tb_event event,
    uint8_t clicks,
    void *user);

struct tb_button {
    tb_config config;
    tb_button_cb callback;
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

tb_config tb_default_config(void);

void tb_init(tb_button *button, const tb_config *config);
void tb_reset(tb_button *button);
void tb_configure(tb_button *button, const tb_config *config);
void tb_set_callback(
    tb_button *button,
    tb_button_cb callback,
    void *user);

void tb_update(tb_button *button, bool pressed, uint32_t now_ms);

bool tb_is_pressed(const tb_button *button);
bool tb_is_long_pressed(const tb_button *button);

#ifdef __cplusplus
}
#endif

#endif
