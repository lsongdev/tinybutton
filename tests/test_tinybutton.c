#include "tinybutton.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    tb_event_t event;
    uint8_t clicks;
} recorded_event_t;

static recorded_event_t events[32];
static unsigned event_count;

static void record_event(
    tb_t *button,
    tb_event_t event,
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

static tb_t make_button(tb_config_t config)
{
    tb_t button;
    event_count = 0;
    tb_init(&button, &config);
    tb_set_callback(&button, record_event, NULL);
    return button;
}

static void expect_event(
    unsigned index,
    tb_event_t event,
    uint8_t clicks)
{
    assert(index < event_count);
    assert(events[index].event == event);
    assert(events[index].clicks == clicks);
}

static void test_debounce_and_single_click(void)
{
    tb_config_t config = {20, 100, 500, 0};
    tb_t button = make_button(config);

    tb_update(&button, false, 0);
    tb_update(&button, true, 10);
    tb_update(&button, false, 15);
    tb_update(&button, true, 20);
    tb_update(&button, true, 39);
    assert(event_count == 0);

    tb_update(&button, true, 40);
    expect_event(0, TB_EVENT_PRESS, 0);

    tb_update(&button, false, 50);
    tb_update(&button, false, 70);
    expect_event(1, TB_EVENT_RELEASE, 0);

    tb_update(&button, false, 169);
    assert(event_count == 2);
    tb_update(&button, false, 170);

    assert(event_count == 3);
    expect_event(2, TB_EVENT_CLICK, 1);
}

static void test_multi_click(void)
{
    tb_config_t config = {0, 100, 500, 0};
    tb_t button = make_button(config);

    tb_update(&button, false, 0);

    tb_update(&button, true, 10);
    tb_update(&button, false, 20);
    tb_update(&button, true, 50);
    tb_update(&button, false, 60);
    tb_update(&button, true, 90);
    tb_update(&button, false, 100);
    tb_update(&button, true, 130);
    tb_update(&button, false, 140);

    tb_update(&button, false, 239);
    assert(event_count == 8);

    tb_update(&button, false, 240);
    assert(event_count == 9);
    expect_event(8, TB_EVENT_CLICK, 4);
}

static void test_long_press_repeat_and_release(void)
{
    tb_config_t config = {0, 100, 200, 50};
    tb_t button = make_button(config);

    tb_update(&button, false, 0);
    tb_update(&button, true, 10);
    tb_update(&button, true, 209);
    assert(event_count == 1);

    tb_update(&button, true, 210);
    expect_event(1, TB_EVENT_LONG_PRESS, 0);

    tb_update(&button, true, 259);
    assert(event_count == 2);

    tb_update(&button, true, 260);
    expect_event(2, TB_EVENT_LONG_REPEAT, 0);

    tb_update(&button, true, 360);
    expect_event(3, TB_EVENT_LONG_REPEAT, 0);

    tb_update(&button, false, 400);
    assert(event_count == 6);
    expect_event(4, TB_EVENT_RELEASE, 0);
    expect_event(5, TB_EVENT_LONG_RELEASE, 0);

    tb_update(&button, false, 1000);
    assert(event_count == 6);
}

static void test_click_then_long_press(void)
{
    tb_config_t config = {0, 100, 200, 0};
    tb_t button = make_button(config);

    tb_update(&button, false, 0);
    tb_update(&button, true, 10);
    tb_update(&button, false, 20);

    tb_update(&button, true, 50);
    tb_update(&button, true, 250);

    assert(event_count == 5);
    expect_event(3, TB_EVENT_CLICK, 1);
    expect_event(4, TB_EVENT_LONG_PRESS, 0);

    tb_update(&button, false, 260);
    assert(event_count == 7);
    expect_event(5, TB_EVENT_RELEASE, 0);
    expect_event(6, TB_EVENT_LONG_RELEASE, 0);
}

static void test_release_candidate_freezes_long_press_deadline(void)
{
    tb_config_t config = {40, 300, 1000, 0};
    tb_t button = make_button(config);

    tb_update(&button, false, 0);

    tb_update(&button, true, 10);
    tb_update(&button, true, 50);
    expect_event(0, TB_EVENT_PRESS, 0);

    /*
     * Start releasing just before the long-press threshold. Once the raw
     * level changes, hold timing must stop even though the release still has
     * to pass debounce.
     */
    tb_update(&button, false, 1040);
    tb_update(&button, false, 1050);
    assert(event_count == 1);

    tb_update(&button, false, 1080);
    assert(event_count == 2);
    expect_event(1, TB_EVENT_RELEASE, 0);

    tb_update(&button, false, 1380);
    assert(event_count == 3);
    expect_event(2, TB_EVENT_CLICK, 1);
}

static void test_press_candidate_freezes_click_deadline(void)
{
    tb_config_t config = {30, 100, 500, 0};
    tb_t button = make_button(config);

    tb_update(&button, false, 0);

    tb_update(&button, true, 10);
    tb_update(&button, true, 40);
    tb_update(&button, false, 50);
    tb_update(&button, false, 80);

    // Start the second press just before the click deadline.
    tb_update(&button, true, 170);
    tb_update(&button, true, 180);
    assert(event_count == 2);

    tb_update(&button, true, 200);
    expect_event(2, TB_EVENT_PRESS, 0);

    tb_update(&button, false, 210);
    tb_update(&button, false, 240);
    tb_update(&button, false, 340);

    assert(event_count == 5);
    expect_event(4, TB_EVENT_CLICK, 2);
}

static void test_timer_wraparound(void)
{
    tb_config_t config = {0, 100, 20, 0};
    tb_t button = make_button(config);

    tb_update(&button, false, UINT32_MAX - 20u);
    tb_update(&button, true, UINT32_MAX - 5u);
    tb_update(&button, true, 13u);
    assert(event_count == 1);

    tb_update(&button, true, 14u);
    assert(event_count == 2);
    expect_event(1, TB_EVENT_LONG_PRESS, 0);
}

static void test_reset_preserves_configuration_and_callback(void)
{
    tb_config_t config = {0, 50, 0, 0};
    tb_t button = make_button(config);

    tb_update(&button, false, 0);
    tb_update(&button, true, 10);
    assert(event_count == 1);

    tb_reset(&button);
    assert(button.config.click_ms == 50);
    assert(button.callback == record_event);

    tb_update(&button, false, 20);
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
