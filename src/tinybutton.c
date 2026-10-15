#include "tinybutton.h"

#include <limits.h>
#include <stddef.h>
#include <string.h>

static uint32_t elapsed(uint32_t now, uint32_t since)
{
    return now - since;
}

static void emit(
    tb_button *button,
    tb_event event,
    uint8_t clicks)
{
    if (button->callback != NULL)
        button->callback(button, event, clicks, button->user);
}

static void finish_clicks(tb_button *button)
{
    if (button->clicks == 0)
        return;

    const uint8_t clicks = button->clicks;
    button->clicks = 0;
    emit(button, TB_EVENT_CLICK, clicks);
}

static void stable_transition(
    tb_button *button,
    bool pressed,
    uint32_t now)
{
    button->pressed = pressed;

    if (pressed)
    {
        button->pressed_at = now;
        button->repeated_at = now;
        button->long_pressed = false;
        emit(button, TB_EVENT_PRESS, 0);
        return;
    }

    emit(button, TB_EVENT_RELEASE, 0);

    if (button->long_pressed)
    {
        button->long_pressed = false;
        button->clicks = 0;
        emit(button, TB_EVENT_LONG_RELEASE, 0);
        return;
    }

    if (button->clicks < UINT8_MAX)
        button->clicks++;

    button->released_at = now;

    if (button->config.click_ms == 0)
        finish_clicks(button);
}

tb_config tb_default_config(void)
{
    const tb_config config = {
        TB_DEFAULT_DEBOUNCE_MS,
        TB_DEFAULT_CLICK_MS,
        TB_DEFAULT_LONG_PRESS_MS,
        TB_DEFAULT_REPEAT_MS,
    };
    return config;
}

void tb_init(
    tb_button *button,
    const tb_config *config)
{
    if (button == NULL)
        return;

    memset(button, 0, sizeof(*button));
    button->config = config != NULL
        ? *config
        : tb_default_config();
}

void tb_reset(tb_button *button)
{
    if (button == NULL)
        return;

    const tb_config config = button->config;
    const tb_button_cb callback = button->callback;
    void *const user = button->user;

    memset(button, 0, sizeof(*button));
    button->config = config;
    button->callback = callback;
    button->user = user;
}

void tb_configure(
    tb_button *button,
    const tb_config *config)
{
    if (button == NULL || config == NULL)
        return;

    button->config = *config;
}

void tb_set_callback(
    tb_button *button,
    tb_button_cb callback,
    void *user)
{
    if (button == NULL)
        return;

    button->callback = callback;
    button->user = user;
}

void tb_update(
    tb_button *button,
    bool pressed,
    uint32_t now)
{
    if (button == NULL)
        return;

    if (!button->initialized)
    {
        button->initialized = true;
        button->raw_pressed = pressed;
        button->pressed = pressed;
        button->raw_changed_at = now;
        button->pressed_at = now;
        button->repeated_at = now;
        return;
    }

    if (pressed != button->raw_pressed)
    {
        button->raw_pressed = pressed;
        button->raw_changed_at = now;
    }

    if (button->raw_pressed != button->pressed)
    {
        if (elapsed(now, button->raw_changed_at) < button->config.debounce_ms)
        {
            /*
             * Freeze gesture timers while an opposite level is being
             * debounced. A release candidate near the long-press threshold
             * must not turn a short press into a long press, and a press
             * candidate near the click deadline must not split a multi-click.
             */
            return;
        }

        stable_transition(button, button->raw_pressed, now);
    }

    if (button->pressed &&
        !button->long_pressed &&
        button->config.long_press_ms != 0 &&
        elapsed(now, button->pressed_at) >= button->config.long_press_ms)
    {
        /*
         * A previous short click followed by a held press is two gestures:
         * finish the completed click sequence before reporting the long press.
         */
        finish_clicks(button);
        button->long_pressed = true;
        button->repeated_at = now;
        emit(button, TB_EVENT_LONG_PRESS, 0);
    }

    if (button->pressed &&
        button->long_pressed &&
        button->config.repeat_ms != 0 &&
        elapsed(now, button->repeated_at) >= button->config.repeat_ms)
    {
        button->repeated_at = now;
        emit(button, TB_EVENT_LONG_REPEAT, 0);
    }

    /*
     * A raw press candidate freezes the click deadline while it is being
     * debounced. This avoids splitting a valid multi-click merely because the
     * debounce interval crosses the click timeout.
     */
    if (!button->pressed &&
        !button->raw_pressed &&
        button->clicks != 0 &&
        elapsed(now, button->released_at) >= button->config.click_ms)
    {
        finish_clicks(button);
    }
}

bool tb_is_pressed(const tb_button *button)
{
    return button != NULL && button->pressed;
}

bool tb_is_long_pressed(const tb_button *button)
{
    return button != NULL && button->long_pressed;
}
