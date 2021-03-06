#include "common.h"

#define NUM_BARS 4
#define INCREASE_RATE 2
#define ACCEL_STEP_MS 50
#define BAR_PX_LENGTH 50
#define PADDING 20
#define BAR_HALF_WIDTH 10
#define BAR_WIDTH 20

typedef enum {
  TOP = 0,
  RIGHT = 1,
  BOTTOM = 2,
  LEFT = 3,
} Location;

typedef struct bar {
  GRect rect;
} Bar;

typedef struct barfill {
  int x;
  int y;
} BarFill;


static Window *window;
static Layer *main_layer;
static TextLayer *ticker_layer;
static TextLayer *text_layer;
static TextLayer *notification_layer;
static AppTimer *timer;
static Bar bars[NUM_BARS];
static uint8_t question_ticker = 1;
static BarFill bfill;

static char question_num_text[10];

static BitmapLayer* check_icon_layer;
static GBitmap *check_icon;

static BitmapLayer* x_icon_layer;
static GBitmap *x_icon;

static BitmapLayer* plus_icon_layer;
static GBitmap *plus_icon;

static BitmapLayer* minus_icon_layer;
static GBitmap *minus_icon;


// globals
extern int32_t question_number;
extern int32_t answer;

static void decrementTicker(uint8_t amount) {
  if (question_ticker - amount < 1) {
    question_ticker = 1;
  } else {
    question_ticker -= amount;
  }
}

static void incrementTicker(uint8_t amount) {

  uint8_t max = 0;
  max = max - 1;


  if (question_ticker + amount > 255) {
    question_ticker = 255;
  } else {
    question_ticker += amount;
  }
}

static void popView() {
  window_stack_pop(false);
}

static void timer_callback(void *data) { 
  static int lastDirection = -1;
  static int timeHold = 0;

  AccelData accel = (AccelData) { .x = 0, .y = 0, .z = 0 };

  accel_service_peek(&accel);
  // Insert UI code here

  //ignore
  if (abs(accel.x) < 200 && abs(accel.y) < 200) {
    bfill.x = 0;
    bfill.y = 0;

    timeHold = 0;

    //accept
  } else {
    if (abs(accel.x) >= abs(accel.y)) {

      //make smoother

      //right
      if (accel.x >= 0) {
        bfill.x += INCREASE_RATE;
        bfill.y = 0;

        //left 
      } else {
        bfill.x -= INCREASE_RATE;
        bfill.y = 0;
      }
      timeHold = 0;
    } else {

      // up 
      if (accel.y >= 0) {
        if (lastDirection == TOP) {
          timeHold += 1;
        } else {
          timeHold = 0;
          lastDirection = TOP;
        }

        // after 3 seconds, increment by 5's
        if (timeHold >= 80) {
          bfill.y = 50;
          if (timeHold % 10 == 0) {
            incrementTicker(10);
          }
        } else if (timeHold >= 40) {
          bfill.y = 34;
          if (timeHold % 10 == 0) {
            incrementTicker(5);
          }
        } else {
          bfill.y = 17;
          if (timeHold %10 == 0) {
            incrementTicker(1);
          }
        }

        lastDirection = TOP;
        bfill.x = 0;

        //down
      } else {
        if (lastDirection == BOTTOM) {
          timeHold += 1;
        } else {
          lastDirection = BOTTOM;
          timeHold = 0;
        }

        // after 3 seconds, increment by 5's
        if (timeHold >= 80) {
          bfill.y = -50;
          if (timeHold % 10 == 0) {
            decrementTicker(10);
          }
        } else if (timeHold >= 40) {
          bfill.y = -34;
          if (timeHold % 10 == 0) {
            decrementTicker(5);
          }
        } else {
          bfill.y = -17;
          if (timeHold %10 == 0) {
            decrementTicker(1);
          }
        }

        lastDirection = BOTTOM;
        bfill.x = 0;
      }
    }
  }

  if (bfill.x >= 50) {
    question_number = question_ticker;
    send_msg( question_number, -1);
    text_layer_set_text(notification_layer, "Request\nSent");
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(notification_layer)); 
    app_timer_register(2000, popView, NULL);
  } else if (bfill.x <= -50) {
    text_layer_set_text(notification_layer, "Request\nCancelled");
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(notification_layer)); 
    app_timer_register(2000, popView, NULL);
  } else {
    layer_mark_dirty(main_layer);
    snprintf( question_num_text, 10, "Q: %d", question_ticker);
    text_layer_set_text(ticker_layer, question_num_text);
    timer = app_timer_register(ACCEL_STEP_MS, timer_callback, NULL);  
  }

  
}

static void main_layer_update_callback(Layer *me, GContext *ctx) {
  for (int i=0; i<4; i++) {
    graphics_draw_rect(ctx, bars[i].rect);
  }
  // top
  if (bfill.x == 0 && bfill.y > 0) {
    graphics_fill_rect(ctx,
        GRect(bars[0].rect.origin.x,
          bars[0].rect.origin.y + BAR_PX_LENGTH,
          bars[0].rect.size.w,
          0 - bfill.y),
        0,
        GCornerNone);
    // right
  } else if (bfill.x > 0 && bfill.y == 0) {
    graphics_fill_rect(ctx,
        GRect(bars[1].rect.origin.x,
          bars[1].rect.origin.y,
          bfill.x,
          bars[1].rect.size.h),
        0,
        GCornerNone);
    // bottom
  } else if (bfill.x == 0 && bfill.y < 0) {
    graphics_fill_rect(ctx,
        GRect(bars[2].rect.origin.x,
          bars[2].rect.origin.y,
          bars[2].rect.size.w,
          0 - bfill.y),
        0,
        GCornerNone);
    // left
  } else if (bfill.x < 0 && bfill.y == 0) {
    graphics_fill_rect(ctx,
        GRect(bars[3].rect.origin.x + BAR_PX_LENGTH,
          bars[3].rect.origin.y,
          bfill.x,
          bars[3].rect.size.h),
        0,
        GCornerNone);
  } else {
    ;
  }
}

static void draw_letters(Layer *window_layer, GRect bounds) {
  text_layer = text_layer_create((GRect) { .origin = { bounds.size.w/2 - 10 - 50, bounds.size.h/2 - 30}, .size = { 50, 20 } });
  text_layer_set_text(text_layer, "Cancel");
  text_layer_set_text_alignment(text_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));

  text_layer = text_layer_create((GRect) { .origin = { bounds.size.w/2 + 10 , bounds.size.h/2 - 30}, .size = { 50, 20 } });
  text_layer_set_text(text_layer, "Accept");
  text_layer_set_text_alignment(text_layer, GTextAlignmentRight );
  layer_add_child(window_layer, text_layer_get_layer(text_layer));

  text_layer = text_layer_create((GRect) { .origin = { bounds.size.w/2 +17, 0}, .size = { 50, 50 } });
  text_layer_set_text(text_layer, "Request\nAnswer");
  text_layer_set_text_alignment(text_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));

  ticker_layer = text_layer_create((GRect) { .origin = { 5, 0 }, .size = { 55, 20 } });
  snprintf( question_num_text, 10, "Q: %d", question_ticker);
  text_layer_set_text(ticker_layer, question_num_text);
  text_layer_set_text_alignment(ticker_layer, GTextAlignmentLeft);
  text_layer_set_font(ticker_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(ticker_layer));
}

static void window_load( Window * window ) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "INSIDE FILL REQUEST\n" );

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  main_layer = layer_create(bounds);
  layer_set_update_proc(main_layer, main_layer_update_callback);
  layer_add_child(window_layer, main_layer);

  draw_letters(window_layer, bounds);

  int halfWidth = bounds.size.w/2;
  int halfHeight = bounds.size.h/2;

  bars[0].rect = GRect(halfWidth-BAR_HALF_WIDTH , halfHeight - BAR_HALF_WIDTH - BAR_PX_LENGTH + 1, BAR_WIDTH , BAR_PX_LENGTH);
  bars[1].rect = GRect(halfWidth + BAR_HALF_WIDTH -1 , halfHeight - BAR_HALF_WIDTH , BAR_PX_LENGTH , BAR_WIDTH);
  bars[2].rect = GRect(halfWidth-BAR_HALF_WIDTH , halfHeight + BAR_HALF_WIDTH - 1, BAR_WIDTH , BAR_PX_LENGTH);
  bars[3].rect = GRect(halfWidth - BAR_HALF_WIDTH - BAR_PX_LENGTH + 1, halfHeight-BAR_HALF_WIDTH , BAR_PX_LENGTH , BAR_WIDTH);

  check_icon_layer = bitmap_layer_create(GRect(halfWidth + 43,halfHeight-6,12,12));
  check_icon = gbitmap_create_with_resource(RESOURCE_ID_CHECK_ICON);
  bitmap_layer_set_bitmap(check_icon_layer, check_icon);
  layer_add_child(window_layer, bitmap_layer_get_layer(check_icon_layer));

  x_icon_layer = bitmap_layer_create(GRect(halfWidth - 55,halfHeight-6,12,12));
  x_icon = gbitmap_create_with_resource(RESOURCE_ID_X_ICON);
  bitmap_layer_set_bitmap(x_icon_layer, x_icon);
  layer_add_child(window_layer, bitmap_layer_get_layer(x_icon_layer));

  plus_icon_layer = bitmap_layer_create(GRect(halfWidth - 6,halfHeight - 55,12,12));
  plus_icon = gbitmap_create_with_resource(RESOURCE_ID_PLUS_ICON);
  bitmap_layer_set_bitmap(plus_icon_layer, plus_icon);
  layer_add_child(window_layer, bitmap_layer_get_layer(plus_icon_layer));

  minus_icon_layer = bitmap_layer_create(GRect(halfWidth - 6,halfHeight + 43,12,12));
  minus_icon = gbitmap_create_with_resource(RESOURCE_ID_MINUS_ICON);
  bitmap_layer_set_bitmap(minus_icon_layer, minus_icon);
  layer_add_child(window_layer, bitmap_layer_get_layer(minus_icon_layer));

  notification_layer = text_layer_create((GRect) { .origin = { 10, 20 }, .size = { 124, 60 } });
  text_layer_set_background_color( notification_layer, GColorBlack );
  text_layer_set_text_alignment(notification_layer, GTextAlignmentCenter);
  text_layer_set_font(notification_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_color( notification_layer, GColorWhite );

  question_ticker = 1;

  timer = app_timer_register(ACCEL_STEP_MS, timer_callback, NULL); 
}

static void window_unload( Window * window ) {
  question_ticker = 1;

  gbitmap_destroy(check_icon);
  gbitmap_destroy(x_icon);
  gbitmap_destroy(plus_icon);
  gbitmap_destroy(minus_icon);
  text_layer_destroy( text_layer );
  text_layer_destroy( ticker_layer );
  text_layer_destroy( notification_layer );
  layer_destroy(main_layer);
}

static void window_appear( Window * window) {
  accel_data_service_subscribe(0, NULL);
}

static void window_disappear( Window * window) {
  accel_data_service_unsubscribe();
  app_timer_cancel(timer);
}

void send_request_init(void) {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
    .appear = window_appear,
    .disappear = window_disappear
  });
  const bool animated = true;
  window_stack_push(window, animated);

  bfill.x = 0;
  bfill.y = 0;
}