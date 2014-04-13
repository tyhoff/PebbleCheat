#include "common.h"

#define NUM_BARS 2
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
static BarFill bfill;

// globals
extern int32_t question_number;
extern int32_t answer;

static void timer_callback(void *data) { 

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
    }
  }

  // accept threshold hit
  if (bfill.x >= 50) {
    window_stack_pop(false);
    fill_request_init();

  //deny threshold hit
  } else  if (bfill.x <= -50) {
    window_stack_pop(true);

  // keep drawing, threshold not hit
  } else {
    layer_mark_dirty(main_layer);
    timer = app_timer_register(ACCEL_STEP_MS, timer_callback, NULL);
  }

  /* APP_LOG(APP_LOG_LEVEL_DEBUG, "x: %d y: %d", accel.x, accel.y); */

  //compute where the next progress and direction should be

 
}

static void main_layer_update_callback(Layer *me, GContext *ctx) {
  for (int i=0; i<4; i++) {
    graphics_draw_rect(ctx, bars[i].rect);
  }
  // right
  if (bfill.x > 0 && bfill.y == 0) {
    graphics_fill_rect(ctx,
        GRect(bars[0].rect.origin.x,
          bars[0].rect.origin.y,
          bfill.x,
          bars[0].rect.size.h),
        0,
        GCornerNone);
    // left
  } else if (bfill.x < 0 && bfill.y == 0) {
    graphics_fill_rect(ctx,
        GRect(bars[1].rect.origin.x + BAR_PX_LENGTH,
          bars[1].rect.origin.y,
          bfill.x,
          bars[1].rect.size.h),
        0,
        GCornerNone);
  } else {
    ;
  }
}

static void draw_letters(Layer *window_layer, GRect bounds) {
  text_layer = text_layer_create((GRect) { .origin = { bounds.size.w/2 - 65, 10}, .size = { 130, 20 } });
  text_layer_set_text(text_layer, "Answer Requested");
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));

  text_layer = text_layer_create((GRect) { .origin = { bounds.size.w/2 - 10 - 50, bounds.size.h/2 - 30}, .size = { 50, 20 } });
  text_layer_set_text(text_layer, "Deny");
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));

  text_layer = text_layer_create((GRect) { .origin = { bounds.size.w/2 + 10 , bounds.size.h/2 - 30}, .size = { 50, 20 } });
  text_layer_set_text(text_layer, "Accept");
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

  int halfWidth = bounds.size.w/2;
  int halfHeight = bounds.size.h/2;

  draw_letters(window_layer, bounds);

  //right
  bars[0].rect = GRect(halfWidth + BAR_HALF_WIDTH -1 , halfHeight - BAR_HALF_WIDTH , BAR_PX_LENGTH , BAR_WIDTH);
  
  //left
  bars[1].rect = GRect(halfWidth - BAR_HALF_WIDTH - BAR_PX_LENGTH + 1, halfHeight-BAR_HALF_WIDTH , BAR_PX_LENGTH , BAR_WIDTH);

  // accel_data_service_subscribe(0, NULL);

  timer = app_timer_register(ACCEL_STEP_MS, timer_callback, NULL);
  
}

static void window_unload( Window * window ) {
  // text_layer_destroy( text_layer );
  layer_destroy(main_layer);
  // accel_data_service_unsubscribe();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "tyler deinit: %p", window);
}

static void init(void) {

  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);

  bfill.x = 0;
}

void accept_request_init(){
  init();
}
