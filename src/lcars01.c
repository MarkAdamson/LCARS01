#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include "ctype.h"
#include "stdlib.h"


#define MY_UUID { 0x4C, 0xB2, 0x35, 0x06, 0xB6, 0x85, 0x45, 0xC1, 0x9E, 0xB3, 0x15, 0x35, 0x19, 0xCA, 0x27, 0xD6 }
PBL_APP_INFO(MY_UUID,
             "LCARS01", "Mark Adamson",
             0, 1, /* App version */
             RESOURCE_ID_IMAGE_MENU_ICON,
             APP_INFO_WATCH_FACE);

Window window;

BmpContainer bg;

GFont lcarsTimeFont;
GFont lcarsDateFont;

int secs;

TextLayer timeLayer; // The clock
TextLayer dateLayer; //The date
TextLayer dayLayer; //the day (duh)
Layer binClock;


void set_container_image(BmpContainer *bmp_container, const int resource_id, GPoint origin) {

  layer_remove_from_parent(&bmp_container->layer.layer);
  bmp_deinit_container(bmp_container);

  bmp_init_container(resource_id, bmp_container);

  GRect frame = layer_get_frame(&bmp_container->layer.layer);
  frame.origin.x = origin.x;
  frame.origin.y = origin.y;
  layer_set_frame(&bmp_container->layer.layer, frame);

  layer_add_child(&window.layer, &bmp_container->layer.layer);
}

void uppercase( char *sPtr )
{
  while ( *sPtr != '\0' ) {
    *sPtr = toupper ( ( unsigned char ) *sPtr );
    ++sPtr;
  }
}

// inserts into subject[] at position pos
void append(char subject[], const char insert[], int pos) {
    char buf[100] = {}; // 100 so that it's big enough. fill with 0
    // or you could use malloc() to allocate sufficient space

    strncpy(buf, subject, pos); // copy at most first pos characters
    int len = strlen(buf);
    strcpy(buf+len, insert); // copy all of insert[] at the end
    len += strlen(insert);  // increase the length by length of insert[]
    strcpy(buf+len, subject+pos); // copy the rest

    strcpy(subject, buf);   // copy it back to subject
    // deallocate buf[] here, if used malloc()
}

void addspaces(char subject[] )
{
	//char buf[strlen(subject) + strlen(subject) -1] = {};
	
}

// Called once per second
void handle_second_tick(AppContextRef ctx, PebbleTickEvent *t) {

  (void)t;
  (void)ctx;

  static char timeText[] = "00 00"; // Needs to be static because it's used by the system later.
  static char dateText[] = "MMM DD";
  static char dayText[] = "WEDNESDAY";

  PblTm currentTime;


  get_time(&currentTime);

  string_format_time(timeText, sizeof(timeText), "%H %M", &currentTime);
  string_format_time(dateText, sizeof(dateText), "%b %d", &currentTime);
  string_format_time(dayText, sizeof(dayText), "%A", &currentTime);
  
  uppercase(dateText);
  uppercase(dayText);

  text_layer_set_text(&timeLayer, timeText);
  text_layer_set_text(&dateLayer, dateText);
	text_layer_set_text(&dayLayer, dayText);
}

void drawbar(GContext* ctx, int col, int row) {
	graphics_fill_rect(ctx, GRect(col*14, row*3, 12, 2), 0, GCornerNone);
}

void drawcolumn(GContext* ctx, int number, int height) {
	for(int i=9;i >= 0; i--) {
		if(height > i) drawbar(ctx, number, 9-i);
	}
}

void drawbinarycolumn(GContext* ctx, int number, bool value) {
	//if (value) drawcolumn(ctx, number, 10-rand() %4);
	//else drawcolumn(ctx, number, rand() % 4);
}

void binClock_update_callback(Layer *me, GContext* ctx ) {
	PblTm t;
	get_time(&t);
    
	graphics_context_set_fill_color(ctx, GColorBlack);
	graphics_fill_rect(ctx, GRect(0,0,45,119), 0, GCornerNone);
    graphics_context_set_fill_color(ctx, GColorWhite);
	for(int i = 0; i < 6; i++) drawcolumn (ctx, i, (t.tm_sec >> (5 - i)  & 1) ? 10 - (i % 3) : i % 4);
	//drawcolumn(ctx, i, (i % 2) ? i % 4 : 10 - (i % 4));
}


void handle_init(AppContextRef ctx) {
  // Create our app's base window
  window_init(&window, "LCARS01");
  window_stack_push(&window, true);
  window_set_background_color(&window, GColorBlack);

  resource_init_current_app(&APP_RESOURCES);

  bmp_init_container(RESOURCE_ID_IMAGE_BACKGROUND, &bg);
  layer_add_child(&window.layer, &bg.layer.layer);
  //set_container_image(&bg, RESOURCE_ID_IMAGE_BACKGROUND, GPoint(5,9));
  
  layer_init(&binClock, GRect(45,119,82,29));
  binClock.update_proc = &binClock_update_callback;
  layer_add_child(&bg.layer.layer, &binClock);

  lcarsTimeFont = \
    fonts_load_custom_font(resource_get_handle(RESOURCE_ID_LCARS_TIME_FONT_60));
  lcarsDateFont = \
    fonts_load_custom_font(resource_get_handle(RESOURCE_ID_LCARS_DATE_FONT_18));

  // Init the text layer used to show the time
  // TODO: Wrap this boilerplate in a function?
  text_layer_init(&timeLayer, GRect(40, 18, 98 /* width */, 163 /* height */));
  text_layer_set_text_color(&timeLayer, GColorWhite);
  text_layer_set_background_color(&timeLayer, GColorClear);
  text_layer_set_font(&timeLayer, lcarsTimeFont);
  
  text_layer_init(&dateLayer, GRect(89,4,47,20));
  text_layer_set_text_color(&dateLayer, GColorWhite);
  text_layer_set_background_color(&dateLayer, GColorClear);
  text_layer_set_font(&dateLayer, lcarsDateFont);
  
  text_layer_init(&dayLayer, GRect(40,90,90,20));
  text_layer_set_text_color(&dayLayer, GColorWhite);
  text_layer_set_background_color(&dayLayer, GColorClear);
  text_layer_set_text_alignment(&dayLayer, GTextAlignmentCenter);
  text_layer_set_font(&dayLayer, lcarsDateFont);

  // Ensures time is displayed immediately (will break if NULL tick event accessed).
  // (This is why it's a good idea to have a separate routine to do the update itself.)
  handle_second_tick(ctx, NULL);

  layer_add_child(&bg.layer.layer, &timeLayer.layer);
  layer_add_child(&bg.layer.layer, &dateLayer.layer);
  layer_add_child(&bg.layer.layer, &dayLayer.layer);
}


void handle_deinit(AppContextRef ctx) {
  (void)ctx;

  bmp_deinit_container(&bg);

}


void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
	.deinit_handler = &handle_deinit,

    // Handle time updates
    .tick_info = {
      .tick_handler = &handle_second_tick,
      .tick_units = SECOND_UNIT
    }
  };
  app_event_loop(params, &handlers);
}
