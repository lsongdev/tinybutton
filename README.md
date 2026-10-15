# tinybutton

A small, portable button state machine for embedded systems.

The core is plain C. It does not know about GPIO, Arduino, RTOS tasks, interrupts,
or a particular clock source. Feed it the logical pressed state and a millisecond
timestamp; it turns that stream into button events.

```text
GPIO / input driver
       |
       v
 pressed + time
       |
       v
   tinybutton.c
       |
       +-- press / release
       +-- click x N
       +-- long press
       +-- long repeat
       +-- long release
```

An optional header-only C++ wrapper adds the familiar Arduino GPIO + `millis()`
experience without changing the core.

## Why

Buttons look simple until every project reimplements the same details:
debouncing, click windows, double/triple/multi-click, long press, release, and
repeat behavior.

`tinybutton.c` keeps that policy in one deterministic state machine:

- plain C11 core
- no dynamic allocation
- no hidden global state
- no hardware dependency
- arbitrary multi-click count (1-255)
- wrap-safe 32-bit millisecond timestamps
- optional zero-allocation Arduino C++ wrapper
- host tests with no GPIO mocks required for the core

## C API

```c
#include "tinybutton.h"

static tb_button button;

static void on_button(
    tb_button *button,
    tb_event event,
    uint8_t clicks,
    void *user)
{
    (void)button;
    (void)user;

    if (event == TB_EVENT_CLICK) {
        // clicks == 1, 2, 3, 4, ...
    }
}

void app_init(void)
{
    tb_config config = tb_default_config();

    tb_init(&button, &config);
    tb_set_callback(&button, on_button, NULL);
}

void app_loop(void)
{
    bool pressed = board_button_pressed();
    uint32_t now_ms = board_uptime_ms();

    tb_update(&button, pressed, now_ms);
}
```

The `pressed` argument is logical, not electrical. Active-low GPIO, pull-ups,
interrupts, polling, and pin configuration belong to the platform layer.

This makes the same core usable from Arduino, ESP-IDF, STM32 HAL, NuttX, Zephyr,
FreeRTOS, bare-metal firmware, or any other environment that can provide a
boolean input and a monotonic millisecond counter.

## Events

| Event | Meaning |
| --- | --- |
| `TB_EVENT_PRESS` | Debounced transition to pressed |
| `TB_EVENT_RELEASE` | Debounced transition to released |
| `TB_EVENT_CLICK` | A completed short-click sequence; `clicks` contains the count |
| `TB_EVENT_LONG_PRESS` | The hold threshold was reached |
| `TB_EVENT_LONG_REPEAT` | Optional periodic event while held |
| `TB_EVENT_LONG_RELEASE` | Release after a long press |

A single, double, triple, or four-click gesture is the same event with a
different count. The API does not grow a separate callback for every count.

A short click followed by a held press is treated as two gestures: the completed
click sequence is emitted before `TB_EVENT_LONG_PRESS`.

## Timing

Defaults:

| Setting | Default | Notes |
| --- | ---: | --- |
| debounce | 30 ms | Stable time required before a level transition |
| click interval | 300 ms | Wait after release for another click |
| long press | 800 ms | Hold time before long press |
| repeat | disabled | Set a non-zero interval to enable |

```c
tb_config config = {
    .debounce_ms = 20,
    .click_ms = 250,
    .long_press_ms = 700,
    .repeat_ms = 200,
};
```

Set `long_press_ms` to zero to disable long-press detection. Set
`repeat_ms` to zero to disable long-repeat events. Set `click_ms` to zero to
emit every short click immediately.

The first call to `tb_update()` establishes the initial state without
emitting a synthetic press or release event. If the button boots held, long
press timing starts from that first sample.

## Arduino / C++

The wrapper is intentionally thin:

```cpp
#include <tinybutton.hpp>

TinyButton button(0); // active LOW, INPUT_PULLUP

void setup()
{
    Serial.begin(115200);

    button.onClicks([](uint8_t count) {
        Serial.printf("clicks: %u\n", count);
    });

    button.onLongPress([]() {
        Serial.println("long press");
    });

    button.begin();
}

void loop()
{
    button.tick();
}
```

For other electrical arrangements:

```cpp
TinyButton button(pin, HIGH, INPUT);
```

The wrapper uses function pointers rather than `std::function`, so it does not
need heap allocation. Captureless lambdas work directly.

## Installation

PlatformIO:

```ini
lib_deps =
    https://github.com/lsongdev/tinybutton.git
```

Arduino IDE users can install the repository as a ZIP library.

Pure C projects can compile `src/tinybutton.c` and include `src/tinybutton.h`
directly.

## Development

```sh
make test
```

The test suite builds the C core with `-std=c11 -Wall -Wextra -Werror -pedantic`
and separately compiles the Arduino C++ wrapper against a tiny host stub.

## Version 3

Version 3 is a deliberate breaking rewrite. The old Arduino-only `TinyButton.cpp`
API is not retained as a compatibility layer.

The original KButton project was inspired by the OneButton approach. Version 3
rebuilds the implementation around a platform-neutral C state machine and keeps
Arduino support as an optional layer above it.

## License

MIT

## Name and migration

This project was formerly `kbutton.c` / KButton. It is now `tinybutton`, a
sibling library of [tinymenu](https://github.com/lsongdev/tinymenu). It remains
independent: GPIO sampling and UI message mapping belong to applications.

Include `tinybutton.h` for C or `tinybutton.hpp` for the optional Arduino wrapper.
The C API uses `tb_*` types/functions and `TB_*` event constants;
the C++ class is `TinyButton`. Update existing callers to these names.
