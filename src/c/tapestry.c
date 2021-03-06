#include <pebble.h>
#include <pebble-events/pebble-events.h>
#include <pebble-connection-vibes/connection-vibes.h>
#include <pebble-hourly-vibes/hourly-vibes.h>
#include "logging.h"
#include "colors.h"
#include "tapestry-layer.h"
#include "time-layer.h"
#include "battery-layer.h"
#include "status-layer.h"
#include "banner-layer.h"
#include "date-layer.h"

static Window *s_window;
static TapestryLayer *s_tapestry_layer;
static TimeLayer *s_time_layer;
static BatteryLayer *s_battery_layer;
static StatusLayer *s_status_layer;
static BannerLayer *s_banner_layer;
static DateLayer *s_date_layer;

static EventHandle s_accel_tap_event_handle;
static bool s_animated = false;

static EventHandle s_settings_event_handle;

static void prv_settings_handler(void *context) {
    log_func();
    window_set_background_color(s_window, colors_get_background_color());
    connection_vibes_set_state(atoi(enamel_get_CONNECTION_VIBE()));
    hourly_vibes_set_enabled(enamel_get_HOURLY_VIBE());
#ifdef PBL_HEALTH
    connection_vibes_enable_health(enamel_get_ENABLE_HEALTH());
    hourly_vibes_enable_health(enamel_get_ENABLE_HEALTH());
#endif

    layer_mark_dirty(window_get_root_layer(s_window));
}

static void prv_animation_stopped(Animation *animation, bool finished, void *context) {
    log_func();
    s_animated = false;
}

static void prv_accel_tap_handler(AccelAxisType axis, int32_t direction) {
    log_func();
    if (s_animated) return;
    s_animated = true;

    GRect from = layer_get_frame(s_tapestry_layer);
    uint8_t h = from.size.h / 6;
    GRect to = GRect(from.origin.x, from.origin.y + h, from.size.w, from.size.h);
    PropertyAnimation *animation = property_animation_create_layer_frame(s_tapestry_layer, &from, &to);

    Animation *clone = animation_clone(property_animation_get_animation(animation));
    animation_set_reverse(clone, true);
    animation_set_delay(clone, 5000);

    Animation *sequence = animation_sequence_create(property_animation_get_animation(animation), clone, NULL);
    animation_set_handlers(sequence, (AnimationHandlers) {
        .stopped = prv_animation_stopped
    }, NULL);
    animation_schedule(sequence);
}

static void prv_window_load(Window *window) {
    log_func();
    Layer *root_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(root_layer);
    uint8_t h = bounds.size.h / 6;

    s_tapestry_layer = tapestry_layer_create(GRect(PBL_IF_RECT_ELSE(30, 50), 0, bounds.size.w - PBL_IF_RECT_ELSE(60, 100), bounds.size.h - h));
    GRect frame = layer_get_frame(s_tapestry_layer);
    frame.origin.y -= h;
    layer_set_frame(s_tapestry_layer, frame);

    GRect rect = layer_get_bounds(s_tapestry_layer);
    h = rect.size.h / 6;
    s_time_layer = time_layer_create(GRect(5, h + PBL_IF_RECT_ELSE(10, 22), rect.size.w - 10, rect.size.h - h - PBL_IF_RECT_ELSE(10, 22)), s_tapestry_layer);

    s_battery_layer = battery_layer_create(GRect(5, PBL_IF_RECT_ELSE(2, 8), frame.size.w - 10, 22));
    layer_add_child(s_tapestry_layer, s_battery_layer);

    s_status_layer = status_layer_create(GRect(7, PBL_IF_RECT_ELSE(1, 4), frame.size.w - 14, 25));
    layer_add_child(s_tapestry_layer, s_status_layer);

    h = bounds.size.h / 4;
    s_banner_layer = banner_layer_create(GRect(PBL_IF_RECT_ELSE(5, 30), bounds.size.h - h, bounds.size.w - PBL_IF_RECT_ELSE(10, 60), h - 10));
#ifdef PBL_ROUND
    frame = layer_get_frame(s_banner_layer);
    frame.origin.y -= 10;
    layer_set_frame(s_banner_layer, frame);
#endif

    bounds = layer_get_bounds(s_banner_layer);
    s_date_layer = date_layer_create(GRect(22, 12, bounds.size.w - 44, bounds.size.h - 24), s_banner_layer);

    layer_add_child(root_layer, s_banner_layer);
    layer_add_child(root_layer, s_tapestry_layer);

    s_accel_tap_event_handle = events_accel_tap_service_subscribe(prv_accel_tap_handler);

    prv_settings_handler(NULL);
    s_settings_event_handle = enamel_settings_received_subscribe(prv_settings_handler, NULL);
}

static void prv_window_unload(Window *window) {
    log_func();
    enamel_settings_received_unsubscribe(s_settings_event_handle);
    events_accel_tap_service_unsubscribe(s_accel_tap_event_handle);

    date_layer_destroy(s_date_layer);
    banner_layer_destroy(s_banner_layer);
    status_layer_destroy(s_status_layer);
    battery_layer_destroy(s_battery_layer);
    time_layer_destroy(s_time_layer);
    tapestry_layer_destroy(s_tapestry_layer);
}

static void prv_init(void) {
    log_func();
    enamel_init();
    connection_vibes_init();
    hourly_vibes_init();
    uint32_t const pattern[] = { 100 };
    hourly_vibes_set_pattern((VibePattern) {
        .durations = pattern,
        .num_segments = 1
    });

    events_app_message_open();

    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers) {
        .load = prv_window_load,
        .unload = prv_window_unload
    });
    window_stack_push(s_window, true);
}

static void prv_deinit(void) {
    log_func();
    window_destroy(s_window);

    hourly_vibes_deinit();
    connection_vibes_deinit();
    enamel_deinit();
}

int main(void) {
    log_func();
    prv_init();
    app_event_loop();
    prv_deinit();
}
