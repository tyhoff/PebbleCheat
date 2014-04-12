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
static TextLayer *text_layer;
static AppTimer *timer;
static Bar bars[NUM_BARS];
static int count = 0;
static BarFill bfill;

static uint8_t question_requested;

static void out_sent_handler(DictionaryIterator *sent, void *context) {
  // outgoing message was delivered
}

static void out_failed_handler( DictionaryIterator * failed,
                                AppMessageResult reason,
                                void * context ) {
  // outgoing message failed
}

static void in_received_handler( DictionaryIterator * received, 
                                 void * context ) {
  // incoming message received
}

static void in_dropped_handler( AppMessageResult reason,
                                void * context ) {
  // incoming message dropped
}

static void timer_callback(void *data) { 
  static int lastDirection = -1;

  AccelData accel = (AccelData) { .x = 0, .y = 0, .z = 0 };

  accel_service_peek(&accel);
  // Insert UI code here

  //ignore
  if (abs(accel.x) < 200 && abs(accel.y) < 200) {
    bfill.x = 0;
    bfill.y = 0;

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
    } else {

      // up 
      if (accel.y >= 0) {
        bfill.y += INCREASE_RATE;
        bfill.x = 0;

        //down
      } else {
        bfill.y -= INCREASE_RATE;
        bfill.x = 0;
      }
    }
  }

  /* APP_LOG(APP_LOG_LEVEL_DEBUG, "x: %d y: %d", accel.x, accel.y); */

  //compute where the next progress and direction should be

  layer_mark_dirty(main_layer);

  timer = app_timer_register(ACCEL_STEP_MS, timer_callback, NULL);
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
  text_layer = text_layer_create((GRect) { .origin = { bounds.size.w/2 - BAR_HALF_WIDTH, 0}, .size = { 20, 20 } });
  text_layer_set_text(text_layer, "A");
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));

  text_layer = text_layer_create((GRect) { .origin = { 0, bounds.size.h/2 - BAR_HALF_WIDTH}, .size = { 20, 20 } });
  text_layer_set_text(text_layer, "B");
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));

  text_layer = text_layer_create((GRect) { .origin = { bounds.size.w/2 - BAR_HALF_WIDTH, bounds.size.h - 20}, .size = { 20, 20 } });
  text_layer_set_text(text_layer, "C");
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));

  text_layer = text_layer_create((GRect) { .origin = { bounds.size.w - 20, bounds.size.h/2 - BAR_HALF_WIDTH}, .size = { 20, 20 } });
  text_layer_set_text(text_layer, "D");
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
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

  timer = app_timer_register(ACCEL_STEP_MS, timer_callback, NULL);
  
}

static void window_unload( Window * window ) {
  text_layer_destroy( text_layer );
  layer_destroy(main_layer);
}

static void init(void) {

  /* register message handlers  */
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_register_outbox_sent(out_sent_handler);
  app_message_register_outbox_failed(out_failed_handler);

  /* message handling init */
  const uint32_t inbound_size = 64;
  const uint32_t outbound_size = 64;
  app_message_open(inbound_size, outbound_size);

  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);

  bfill.x = 0;
  bfill.y = 0;

  accel_data_service_subscribe(0, NULL);

}

static void deinit(void) {
  window_destroy(window);
  accel_data_service_unsubscribe();
}

void fill_request_init( uint8_t question_requested ){
  init();
}