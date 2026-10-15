#ifndef TINYBUTTON_HPP
#define TINYBUTTON_HPP

#include <Arduino.h>

#include "tinybutton.h"

class TinyButton {
public:
    using Callback = void (*)();
    using ClickCallback = void (*)(uint8_t count);

    explicit TinyButton(
        uint8_t pin,
        uint8_t activeLevel = LOW,
        uint8_t pinModeValue = INPUT_PULLUP)
        : pin_(pin),
          active_level_(activeLevel),
          pin_mode_(pinModeValue),
          config_(tb_default_config())
    {
    }

    void begin()
    {
        pinMode(pin_, pin_mode_);
        tb_init(&button_, &config_);
        tb_set_callback(&button_, dispatch, this);
        started_ = true;

        // Establish the initial level without synthesizing a boot-time event.
        tick();
    }

    void tick()
    {
        if (!started_)
            return;

        tb_update(
            &button_,
            digitalRead(pin_) == active_level_,
            millis());
    }

    void reset()
    {
        if (started_)
            tb_reset(&button_);
    }

    void setDebounceMs(uint32_t ms)
    {
        config_.debounce_ms = ms;
        applyConfig();
    }

    void setClickMs(uint32_t ms)
    {
        config_.click_ms = ms;
        applyConfig();
    }

    void setLongPressMs(uint32_t ms)
    {
        config_.long_press_ms = ms;
        applyConfig();
    }

    void setRepeatMs(uint32_t ms)
    {
        config_.repeat_ms = ms;
        applyConfig();
    }

    void onPress(Callback callback)
    {
        press_ = callback;
    }

    void onRelease(Callback callback)
    {
        release_ = callback;
    }

    void onClicks(ClickCallback callback)
    {
        clicks_ = callback;
    }

    void onLongPress(Callback callback)
    {
        long_press_ = callback;
    }

    void onLongRepeat(Callback callback)
    {
        long_repeat_ = callback;
    }

    void onLongRelease(Callback callback)
    {
        long_release_ = callback;
    }

    bool pressed() const
    {
        return tb_is_pressed(&button_);
    }

    bool longPressed() const
    {
        return tb_is_long_pressed(&button_);
    }

    tb_t *core()
    {
        return &button_;
    }

    const tb_t *core() const
    {
        return &button_;
    }

private:
    static void dispatch(
        tb_t *,
        tb_event_t event,
        uint8_t clicks,
        void *user)
    {
        static_cast<TinyButton *>(user)->handle(event, clicks);
    }

    void handle(tb_event_t event, uint8_t clicks)
    {
        switch (event)
        {
        case TB_EVENT_PRESS:
            if (press_ != nullptr)
                press_();
            break;
        case TB_EVENT_RELEASE:
            if (release_ != nullptr)
                release_();
            break;
        case TB_EVENT_CLICK:
            if (clicks_ != nullptr)
                clicks_(clicks);
            break;
        case TB_EVENT_LONG_PRESS:
            if (long_press_ != nullptr)
                long_press_();
            break;
        case TB_EVENT_LONG_REPEAT:
            if (long_repeat_ != nullptr)
                long_repeat_();
            break;
        case TB_EVENT_LONG_RELEASE:
            if (long_release_ != nullptr)
                long_release_();
            break;
        }
    }

    void applyConfig()
    {
        if (started_)
            tb_configure(&button_, &config_);
    }

    uint8_t pin_;
    uint8_t active_level_;
    uint8_t pin_mode_;
    bool started_ = false;

    tb_config_t config_;
    tb_t button_ = {};

    Callback press_ = nullptr;
    Callback release_ = nullptr;
    ClickCallback clicks_ = nullptr;
    Callback long_press_ = nullptr;
    Callback long_repeat_ = nullptr;
    Callback long_release_ = nullptr;
};

#endif
