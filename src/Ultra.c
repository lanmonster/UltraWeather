/******************************************
 TODO:
    -weather
    -batt4 image with no bars in it
 ******************************************/
#include <pebble.h>
#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS 1

static Window *window;

static GBitmap *battery_image;
static BitmapLayer *battery_image_layer;

static TextLayer *condition_layer;
static TextLayer *temperature_layer;

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

static void handle_time_and_date() {
    time_t temp = time(NULL); 
    struct tm *current_time = localtime(&temp);
    char *time_format = "%R";
    static char time_text[6];
    static char date_text[17];
    int weekday = current_time->tm_wday;
    
    if (dayOfWeek_image)
        gbitmap_destroy(dayOfWeek_image);
    
    dayOfWeek_image = gbitmap_create_with_resource(WEEKDAY_RESOURCE_IDS[weekday]);

    if (!clock_is_24h_style())
        time_format = "%I:%M";

    strftime(time_text, sizeof(time_text), time_format, current_time);
    strftime(date_text, sizeof(date_text), "%e %b %Y", current_time);

    text_layer_set_text(date_layer, date_text);
    text_layer_set_text(time_layer, time_text);
}

static void handle_battery() {
    BatteryChargeState charge_state = battery_state_service_peek();
    
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

void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
    handle_time_and_date();
    handle_battery();
    
    // Get weather update every 30 minutes
    if(tick_time->tm_min % 30 == 0) {
        // Begin dictionary
        DictionaryIterator *iter;
        app_message_outbox_begin(&iter);

        // Add a key-value pair
        dict_write_uint8(iter, 0, 0);

        // Send the message!
        app_message_outbox_send();
    }
}

static void window_load(Window *window) {
    //Load time module
    time_layer = text_layer_create(GRect(0, 50, 144, 168));
    text_layer_set_text_color(time_layer, GColorWhite);
    text_layer_set_background_color(time_layer, GColorClear);
    text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
    text_layer_set_font(time_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
    
    //Load weekday module
    dayOfWeek_image = gbitmap_create_with_resource(RESOURCE_ID_MON);
    dayOfWeek_layer = bitmap_layer_create(GRect(0, 40, 144, 168));
    bitmap_layer_set_alignment(dayOfWeek_layer, GAlignCenter);
    bitmap_layer_set_bitmap(dayOfWeek_layer, dayOfWeek_image);
    
    //Load date module
    date_layer = text_layer_create(GRect(0, 140, 144, 168));
    text_layer_set_text_color(date_layer, GColorWhite);
    text_layer_set_background_color(date_layer, GColorClear);
    text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
    text_layer_set_font(date_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
    
    //Load battery module
    battery_image = gbitmap_create_with_resource(RESOURCE_ID_BATT0);
    battery_image_layer = bitmap_layer_create(GRect(5, 10, 144-5, 168-10));
    bitmap_layer_set_alignment(battery_image_layer, GAlignTopRight);
    bitmap_layer_set_bitmap(battery_image_layer, battery_image);
    
    //Load condition module
    condition_layer = text_layer_create(GRect(5, 7, 144, 168));
    text_layer_set_background_color(condition_layer, GColorClear);
    text_layer_set_text_color(condition_layer, GColorWhite);
    text_layer_set_text_alignment(condition_layer, GTextAlignmentCenter);
    text_layer_set_text(condition_layer, "Loading...");
    
    //Load temperature module
    temperature_layer = text_layer_create(GRect(0, 20, 144, 168));
    text_layer_set_text_alignment(temperature_layer, GTextAlignmentCenter);
    text_layer_set_background_color(temperature_layer, GColorClear);
    text_layer_set_text_color(temperature_layer, GColorWhite);
    text_layer_set_font(temperature_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    text_layer_set_text(temperature_layer, "--- F");
    
    //Load weather image module
    
    //Add all layers to window
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(time_layer));
    layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(dayOfWeek_layer));
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(date_layer));
    layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(battery_image_layer));
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(condition_layer));
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(temperature_layer));
    
    //Update time, date, and battery stats
    handle_time_and_date();
    handle_battery();
}

static void window_unload(Window *window) {
    //Destroy time module
    text_layer_destroy(time_layer);
    
    //Destroy weekday module
    gbitmap_destroy(dayOfWeek_image);
    bitmap_layer_destroy(dayOfWeek_layer);
    
    //Destroy date module
    text_layer_destroy(date_layer);
    
    //Destroy battery module
    gbitmap_destroy(battery_image);
    bitmap_layer_destroy(battery_image_layer);
    
    //Destroy condition module
    text_layer_destroy(condition_layer);
    
    //Destroy temperature module
    text_layer_destroy(temperature_layer);
    
    //Destroy weather image module

}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    // Store incoming information
    static char temperature_buffer[8];
    static char conditions_buffer[32];

    // Read first item
    Tuple *t = dict_read_first(iterator);
    
    // For all items
    while(t != NULL) {
        // Which key was received?
        switch(t->key) {
            case KEY_TEMPERATURE:
                snprintf(temperature_buffer, sizeof(temperature_buffer), "%dC", (int)t->value->int32);
                text_layer_set_text(temperature_layer, temperature_buffer);
                break;
            case KEY_CONDITIONS:
                snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", t->value->cstring);
                text_layer_set_text(condition_layer, conditions_buffer);
                break;
            default:
                APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
                break;
        }

        // Look for next item
        t = dict_read_next(iterator);
    }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void init(void) {
    window = window_create();
    window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
    });
    window_set_background_color(window, GColorBlack);
    window_stack_push(window, true);

    battery_state_service_subscribe(&handle_battery);
    tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
    
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);
    
    app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void deinit(void) {
    battery_state_service_unsubscribe();
    tick_timer_service_unsubscribe();

    window_destroy(window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
