#include "tinybutton.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    tinybutton_event_t event;
    uint8_t clicks;
} recorded_event_t;

static recorded_event_t events[32];
static unsigned event_count;

static void record_event(
    tinybutton_t *button,
    tinybutton_event_t event,
    uint8_t clicks,
    void *user)
{
    (void)button;
    (void)user;
    assert(event_count < sizeof(events) / sizeof(events[0]));
    events[event_count].event = event;
    events[event_count].clicks = clicks;
    event_count++;
}

static tinybutton_t make_button(tinybutton_config_t config)
{
    tinybutton_t button;
    event_count = 0;
    tinybutton_init(&button, &config);
    tinybutton_set_callback(&button, record_event, NULL);
    return button;
}

static void expect_event(
    unsigned index,
    tinybutton_event_t event,
    uint8_t clicks)
{
    assert(index < event_count);
    assert(events[index].event == event);
    assert(events[index].clicks == clicks);
}

static void test_debounce_and_single_click(void)
{
    tinybutton_config_t config = {20, 100, 500, 0};
    tinybutton_t button = make_button(config);

    tinybutton_update(&button, false, 0);
    tinybutton_update(&button, true, 10);
    tinybutton_update(&button, false, 15);
    tinybutton_update(&button, true, 20);
    tinybutton_update(&button, true, 39);
    assert(event_count == 0);

    tinybutton_update(&button, true, 40);
    expect_event(0, TINYBUTTON_EVENT_PRESS, 0);

    tinybutton_update(&button, false, 50);
    tinybutton_update(&button, false, 70);
    expect_event(1, TINYBUTTON_EVENT_RELEASE, 0);

    tinybutton_update(&button, false, 169);
    assert(event_count == 2);
    tinybutton_update(&button, false, 170);

    assert(event_count == 3);
    expect_event(2, TINYBUTTON_EVENT_CLICK, 1);
}

static void test_multi_click(void)
{
    tinybutton_config_t config = {0, 100, 500, 0};
    tinybutton_t button = make_button(config);

    tinybutton_update(&button, false, 0);

    tinybutton_update(&button, true, 10);
    tinybutton_update(&button, false, 20);
    tinybutton_update(&button, true, 50);
    tinybutton_update(&button, false, 60);
    tinybutton_update(&button, true, 90);
    tinybutton_update(&button, false, 100);
    tinybutton_update(&button, true, 130);
    tinybutton_update(&button, false, 140);

    tinybutton_update(&button, false, 239);
    assert(event_count == 8);

    tinybutton_update(&button, false, 240);
    assert(event_count == 9);
    expect_event(8, TINYBUTTON_EVENT_CLICK, 4);
}

static void test_long_press_repeat_and_release(void)
{
    tinybutton_config_t config = {0, 100, 200, 50};
    tinybutton_t button = make_button(config);

    tinybutton_update(&button, false, 0);
    tinybutton_update(&button, true, 10);
    tinybutton_update(&button, true, 209);
    assert(event_count == 1);

    tinybutton_update(&button, true, 210);
    expect_event(1, TINYBUTTON_EVENT_LONG_PRESS, 0);

    tinybutton_update(&button, true, 259);
    assert(event_count == 2);

    tinybutton_update(&button, true, 260);
    expect_event(2, TINYBUTTON_EVENT_LONG_REPEAT, 0);

    tinybutton_update(&button, true, 360);
    expect_event(3, TINYBUTTON_EVENT_LONG_REPEAT, 0);

    tinybutton_update(&button, false, 400);
    assert(event_count == 6);
    expect_event(4, TINYBUTTON_EVENT_RELEASE, 0);
    expect_event(5, TINYBUTTON_EVENT_LONG_RELEASE, 0);

    tinybutton_update(&button, false, 1000);
    assert(event_count == 6);
}

static void test_click_then_long_press(void)
{
    tinybutton_config_t config = {0, 100, 200, 0};
    tinybutton_t button = make_button(config);

    tinybutton_update(&button, false, 0);
    tinybutton_update(&button, true, 10);
    tinybutton_update(&button, false, 20);

    tinybutton_update(&button, true, 50);
    tinybutton_update(&button, true, 250);

    assert(event_count == 5);
    expect_event(3, TINYBUTTON_EVENT_CLICK, 1);
    expect_event(4, TINYBUTTON_EVENT_LONG_PRESS, 0);

    tinybutton_update(&button, false, 260);
    assert(event_count == 7);
    expect_event(5, TINYBUTTON_EVENT_RELEASE, 0);
    expect_event(6, TINYBUTTON_EVENT_LONG_RELEASE, 0);
}

static void test_release_candidate_freezes_long_press_deadline(void)
{
    tinybutton_config_t config = {40, 300, 1000, 0};
    tinybutton_t button = make_button(config);

    tinybutton_update(&button, false, 0);

    tinybutton_update(&button, true, 10);
    tinybutton_update(&button, true, 50);
    expect_event(0, TINYBUTTON_EVENT_PRESS, 0);

    /*
     * Start releasing just before the long-press threshold. Once the raw
     * level changes, hold timing must stop even though the release still has
     * to pass debounce.
     */
    tinybutton_update(&button, false, 1040);
    tinybutton_update(&button, false, 1050);
    assert(event_count == 1);

    tinybutton_update(&button, false, 1080);
    assert(event_count == 2);
    expect_event(1, TINYBUTTON_EVENT_RELEASE, 0);

    tinybutton_update(&button, false, 1380);
    assert(event_count == 3);
    expect_event(2, TINYBUTTON_EVENT_CLICK, 1);
}

static void test_press_candidate_freezes_click_deadline(void)
{
    tinybutton_config_t config = {30, 100, 500, 0};
    tinybutton_t button = make_button(config);

    tinybutton_update(&button, false, 0);

    tinybutton_update(&button, true, 10);
    tinybutton_update(&button, true, 40);
    tinybutton_update(&button, false, 50);
    tinybutton_update(&button, false, 80);

    // Start the second press just before the click deadline.
    tinybutton_update(&button, true, 170);
    tinybutton_update(&button, true, 180);
    assert(event_count == 2);

    tinybutton_update(&button, true, 200);
    expect_event(2, TINYBUTTON_EVENT_PRESS, 0);

    tinybutton_update(&button, false, 210);
    tinybutton_update(&button, false, 240);
    tinybutton_update(&button, false, 340);

    assert(event_count == 5);
    expect_event(4, TINYBUTTON_EVENT_CLICK, 2);
}

static void test_timer_wraparound(void)
{
    tinybutton_config_t config = {0, 100, 20, 0};
    tinybutton_t button = make_button(config);

    tinybutton_update(&button, false, UINT32_MAX - 20u);
    tinybutton_update(&button, true, UINT32_MAX - 5u);
    tinybutton_update(&button, true, 13u);
    assert(event_count == 1);

    tinybutton_update(&button, true, 14u);
    assert(event_count == 2);
    expect_event(1, TINYBUTTON_EVENT_LONG_PRESS, 0);
}

static void test_reset_preserves_configuration_and_callback(void)
{
    tinybutton_config_t config = {0, 50, 0, 0};
    tinybutton_t button = make_button(config);

    tinybutton_update(&button, false, 0);
    tinybutton_update(&button, true, 10);
    assert(event_count == 1);

    tinybutton_reset(&button);
    assert(button.config.click_ms == 50);
    assert(button.callback == record_event);

    tinybutton_update(&button, false, 20);
    assert(event_count == 1);
}

int main(void)
{
    test_debounce_and_single_click();
    test_multi_click();
    test_long_press_repeat_and_release();
    test_click_then_long_press();
    test_release_candidate_freezes_long_press_deadline();
    test_press_candidate_freezes_click_deadline();
    test_timer_wraparound();
    test_reset_preserves_configuration_and_callback();

    puts("tinybutton tests passed");
    return 0;
}
