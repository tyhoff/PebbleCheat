#include <pebble.h>
#include "helpers.h"

enum {
  REQUEST_ID        = 0x0,
  QUESTION_NUMBER   = 0x1,
};

void fill_request_init( uint8_t question_requested );
void accept_request_init( uint8_t question_requested );
