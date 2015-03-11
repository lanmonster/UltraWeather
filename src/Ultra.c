/******************************************
 TODO:
    -weather
    -batt4 image with no bars in it
 ******************************************/
#include <pebble.h>

static Window *window;

static GBitmap *bluetooth_image;
static BitmapLayer *bluetooth_image_layer;

static GBitmap *battery_image;
static BitmapLayer *battery_image_layer;
static TextLayer *battery_layer;
static TextLayer *charging_layer;

static TextLayer *time_layer;

static TextLayer *date_layer;

static GBitmap *dayOfWeek_image;
static BitmapLayer *dayOfWeek_layer;

GContext *time_ctx;

const int BATTERY_LEVEL_RESOURCE_IDS[] = {
    RESOURCE_ID_BATT0,
    RESOURCE_ID_BATT1,
    RESOURCE_ID_BATT2,
    RESOURCE_ID_BATT3
};

const int WEEKDAY_RESOURCE_IDS[] = {
    RESOURCE_ID_SUN,
    RESOURCE_ID_MON,
    RESOURCE_ID_TUE,
    RESOURCE_ID_WED,
    RESOURCE_ID_THU,
    RESOURCE_ID_FRI,
    RESOURCE_ID_SAT
};

static void handle_day_of_week(struct tm *current_time) {
    int weekday = current_time->tm_wday;
    if (dayOfWeek_image)
        gbitmap_destroy(dayOfWeek_image);
    dayOfWeek_image = gbitmap_create_with_resource(WEEKDAY_RESOURCE_IDS[weekday]);
}

static void handle_bt() {
    if (bluetooth_image)
        gbitmap_destroy(bluetooth_image);
    if (bluetooth_connection_service_peek())       
        bluetooth_image = gbitmap_create_with_resource(RESOURCE_ID_BLUETOOTH_CONNECTED);
}

static void handle_battery(BatteryChargeState charge_state) {
    static char charging_text[] = "100% charged";
    static char battery_text[] = "000%";

    if (charge_state.is_charging)
        snprintf(charging_text, sizeof(charging_text), "charging");
    else
        snprintf(charging_text, sizeof(charging_text), "discharging");

    snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
    text_layer_set_text(charging_layer, charging_text);
    text_layer_set_text(battery_layer, battery_text);

    if (battery_image)
        gbitmap_destroy(battery_image);
    if (charge_state.charge_percent > 75)
        battery_image = gbitmap_create_with_resource(BATTERY_LEVEL_RESOURCE_IDS[0]);
    else if (charge_state.charge_percent <= 75 && charge_state.charge_percent > 50)
        battery_image = gbitmap_create_with_resource(BATTERY_LEVEL_RESOURCE_IDS[1]);
    else if (charge_state.charge_percent <= 50 && charge_state.charge_percent > 25)
        battery_image = gbitmap_create_with_resource(BATTERY_LEVEL_RESOURCE_IDS[2]);
    else if (charge_state.charge_percent <= 25 && charge_state.charge_percent > 0)
        battery_image = gbitmap_create_with_resource(BATTERY_LEVEL_RESOURCE_IDS[3]);
//     else
//         battery_image = gbitmap_create_with_resource(BATTERY_LEVEL_RESOURCE_IDS[4]);
}

void bluetooth_connection_callback(bool connected) {
    APP_LOG(APP_LOG_LEVEL_INFO, "bluetooth connected=%d", (int) connected);
}

void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
    char *time_format = "%R";

    BatteryChargeState batt = battery_state_service_peek();
    static char time_text[6];
    static char date_text[17];

    if (!clock_is_24h_style())
        time_format = "%I:%M";

    if (!tick_time) {
        time_t now = time(NULL);
        tick_time = localtime(&now);
    }

    strftime(time_text, sizeof(time_text), time_format, tick_time);
    strftime(date_text, sizeof(date_text), "%e %b %Y", tick_time);

    text_layer_set_text(date_layer, date_text);
    text_layer_set_text(time_layer, time_text);

    handle_battery(batt);
    handle_bt();
    handle_day_of_week(tick_time);
}

static void init(void) {
    BatteryChargeState charge_state = battery_state_service_peek();
    static char batt[] = "000%";
    snprintf(batt, sizeof(batt), "%d%%", charge_state.charge_percent);

    window = window_create();
    window_stack_push(window, true);
    window_set_background_color(window, GColorBlack);

    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_frame(window_layer);

    charging_layer = text_layer_create((GRect){ .origin = { 0, 10 }, .size = bounds.size }); //make this more clear
    text_layer_set_text_alignment(charging_layer, GTextAlignmentCenter);
    text_layer_set_background_color(charging_layer, GColorClear);
    text_layer_set_text_color(charging_layer, GColorWhite);

    battery_layer = text_layer_create((GRect){ .origin = { 0, 25 }, .size = bounds.size }); //make this more clear
    text_layer_set_text_alignment(battery_layer, GTextAlignmentCenter);
    text_layer_set_background_color(battery_layer, GColorClear);
    text_layer_set_text_color(battery_layer, GColorWhite);
    text_layer_set_font(battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    text_layer_set_text(battery_layer, batt);

    handle_battery(charge_state);
    
    battery_image_layer = bitmap_layer_create(GRect(5, 10, 144-5, 168-10));
    bitmap_layer_set_alignment(battery_image_layer, GAlignTopRight);
    bitmap_layer_set_bitmap(battery_image_layer, battery_image);

    time_layer = text_layer_create(GRect(7, 50, 144-7, 168-50));
    text_layer_set_text_color(time_layer, GColorWhite);
    text_layer_set_background_color(time_layer, GColorClear);
    text_layer_set_font(time_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));

    date_layer = text_layer_create((GRect){ .origin = { 0, 140 }, .size = bounds.size }); //make this more clear
    text_layer_set_text_color(date_layer, GColorWhite);
    text_layer_set_background_color(date_layer, GColorClear);
    text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
    text_layer_set_font(date_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));

    time_t now = time(NULL);
    struct tm *tick_time = localtime(&now);

    dayOfWeek_image = gbitmap_create_with_resource(RESOURCE_ID_MON);
    dayOfWeek_layer = bitmap_layer_create((GRect){ .origin = { 0, 40 }, .size = bounds.size }); //make this more clear
    bitmap_layer_set_alignment(dayOfWeek_layer, GAlignCenter);
    bitmap_layer_set_bitmap(dayOfWeek_layer, dayOfWeek_image);

    handle_day_of_week(tick_time);

    if (bluetooth_connection_service_peek())
        bluetooth_image = gbitmap_create_with_resource(RESOURCE_ID_BLUETOOTH_CONNECTED);
    
    bluetooth_image_layer = bitmap_layer_create(GRect(5, 10, 144-5, 168-10));
    bitmap_layer_set_alignment(bluetooth_image_layer, GAlignTopLeft);
    bitmap_layer_set_bitmap(bluetooth_image_layer, bluetooth_image);

    battery_state_service_subscribe(&handle_battery);
    bluetooth_connection_service_subscribe(bluetooth_connection_callback);
    tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);

    handle_minute_tick(NULL, MINUTE_UNIT);

    layer_add_child(window_layer, text_layer_get_layer(battery_layer));
    layer_add_child(window_layer, text_layer_get_layer(charging_layer));
    layer_add_child(window_layer, text_layer_get_layer(time_layer));
    layer_add_child(window_layer, text_layer_get_layer(date_layer));

    layer_add_child(window_layer, bitmap_layer_get_layer(battery_image_layer));
    layer_add_child(window_layer, bitmap_layer_get_layer(bluetooth_image_layer));
    layer_add_child(window_layer, bitmap_layer_get_layer(dayOfWeek_layer));
}

static void deinit(void) {
    battery_state_service_unsubscribe();
    bluetooth_connection_service_unsubscribe();
    tick_timer_service_unsubscribe();

    text_layer_destroy(charging_layer);
    text_layer_destroy(battery_layer);
    text_layer_destroy(time_layer);
    text_layer_destroy(date_layer);

    gbitmap_destroy(bluetooth_image);
    bitmap_layer_destroy(bluetooth_image_layer);

    gbitmap_destroy(battery_image);
    bitmap_layer_destroy(battery_image_layer);

    gbitmap_destroy(dayOfWeek_image);
    bitmap_layer_destroy(dayOfWeek_layer);

    window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
