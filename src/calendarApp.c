#include <pebble.h>
#include <math.h>


#define WATCHMODE false


//Send Keys
#define GET_EVENT_DAYS 1
#define GET_SETTINGS 2
//Recieve Keys
#define MONTHYEAR_KEY 1
#define EVENT_DAYS_DATA_KEY 3
#define SETTINGS_KEY 4
//Storage Keys
#define BLACK_KEY 1
#define GRID_KEY 2
#define INVERT_KEY 3
#define SHOWTIME_KEY 4
#define START_OF_WEEK_KEY 5


static bool black = true;
static bool grid = true;
static bool invert = true;
static bool showtime = false;

// First day of the week. Values can be between -6 and 6 
// 0 = weeks start on Sunday
// 1 =  weeks start on Monday
static int start_of_week = 0;


const char daysOfWeek[7][3] = {"S","M","T","W","Th","F","S"};
const char months[12][12] = {"January","Feburary","March","April","May","June","July","August","September","October", "November", "December"};

static int offset = 0;

static Window *window;
static Layer *days_layer;

static TextLayer *timeLayer;
int curHour;
int curMin;
int curSec;

bool calEvents[32] = {  false,false,false,false,false,
                        false,false,false,false,false,
                        false,false,false,false,false,
                        false,false,false,false,false,
                        false,false,false,false,false,
                        false,false,false,false,false,false,false};

char* intToStr(int val){
 	static char buf[32] = {0};
	int i = 30;	
	for(; val && i ; --i, val /= 10)
		buf[i] = "0123456789"[val % 10];
	
	return &buf[i+1];
}
// Calculate what day of the week it was/will be X days from the first day of the month, if mday was a wday
int wdayOfFirstOffset(int wday,int mday,int ofs){
    int a = wday - ((mday-1)%7);
    if(a<0) a += 7;

    int b;
    if(ofs>0)
        b = a + (abs(ofs)%7); 
    else
        b = a - (abs(ofs)%7); 
    
    if(b<0) b += 7;
    if(b>6) b -= 7;
    
    return b;
}

// How many days are/were in the month
int daysInMonth(int mon, int year){
    mon++;
    
    // April, June, September and November have 30 Days
    if(mon == 4 || mon == 6 || mon == 9 || mon == 11)
        return 30;
        
    // Deal with Feburary & Leap years
    else if( mon == 2 ){
        if(year%400==0)
            return 29;
        else if(year%100==0)
            return 28;
        else if(year%4==0)
            return 29;
        else 
            return 28;
    }
    // Most months have 31 days
    else
        return 31;
}
void setColors(GContext* ctx){
    if(black){
        window_set_background_color(window, GColorBlack);
        graphics_context_set_stroke_color(ctx, GColorWhite);
        graphics_context_set_fill_color(ctx, GColorBlack);
        graphics_context_set_text_color(ctx, GColorWhite);
    }else{
        window_set_background_color(window, GColorWhite);
        graphics_context_set_stroke_color(ctx, GColorBlack);
        graphics_context_set_fill_color(ctx, GColorWhite);
        graphics_context_set_text_color(ctx, GColorBlack);
    }
}
void setInvColors(GContext* ctx){
    if(black){
        window_set_background_color(window, GColorWhite);
        graphics_context_set_stroke_color(ctx, GColorBlack);
        graphics_context_set_fill_color(ctx, GColorWhite);
        graphics_context_set_text_color(ctx, GColorBlack);
    }else{
        graphics_context_set_stroke_color(ctx, GColorWhite);
        graphics_context_set_fill_color(ctx, GColorBlack);
        graphics_context_set_text_color(ctx, GColorWhite);
    }
}


void days_layer_update_callback(Layer *me, GContext* ctx) {
    
    int j;
    int i;
    setColors(ctx);
    
    time_t now = time(NULL);
    struct tm *currentTime = localtime(&now);


    int mon = currentTime->tm_mon;
    int year = currentTime->tm_year+1900;
    
    // Figure out which month & year we are going to be looking at based on the selected offset
    // Calculate how many days are between the first of this month and the first of the month we are interested in
    int od = 0;
    j = 0;
    while(j < abs(offset)){
        j++;
        if(offset > 0){
            od = od + daysInMonth(mon,year);
            mon++;
            if(mon>11){
                mon -= 12;
                year++;
            }
        }else{
            mon--;
            if(mon < 0){
                mon += 12;
                year--;
            }
            od -= daysInMonth(mon,year);
        }
    }
    
    // Days in the target month
    int dom = daysInMonth(mon,year);
    
    // Day of the week for the first day in the target month 
    int dow = wdayOfFirstOffset(currentTime->tm_wday,currentTime->tm_mday,od);
    
    // Adjust day of week by specified offset
    dow -= start_of_week;
    if(dow>6) dow-=7;
    if(dow<0) dow+=7;
    
    // Cell geometry
    
    int l = 2;      // position of left side of left column
    int b = 168;    // position of bottom of bottom row
    int d = 7;      // number of columns (days of the week)
    int lw = 20;    // width of columns 
    int w = ceil(((float) dow + (float) dom)/7); // number of weeks this month
    

    int bh = 21;
    if(showtime){
        if(w == 4)      bh = 21;
        else if(w == 5) bh = 17;
        else            bh = 14;
    }else{
        // How tall rows should be depends on how many weeks there are
        if(w == 4)      bh = 30;
        else if(w == 5) bh = 24;
        else            bh = 20;
    }

    int r = l+d*lw; // position of right side of right column
    int t = b-w*bh; // position of top of top row
    int cw = lw-1;  // width of textarea
    int cl = l+1;
    int ch = bh-1;
        

    if(grid){
        // Draw the Gridlines
        // horizontal lines
        for(i=1;i<=w;i++){
            graphics_draw_line(ctx, GPoint(l, b-i*bh), GPoint(r, b-i*bh));
        }
        // vertical lines
        for(i=1;i<d;i++){
            graphics_draw_line(ctx, GPoint(l+i*lw, t), GPoint(l+i*lw, b));
        }
    }

    // Draw days of week
    for(i=0;i<7;i++){
    
        // Adjust labels by specified offset
        j = i+start_of_week;
        if(j>6) j-=7;
        if(j<0) j+=7;
        graphics_draw_text(
            ctx, 
            daysOfWeek[j], 
            fonts_get_system_font(FONT_KEY_GOTHIC_14), 
            GRect(cl+i*lw, b-w*bh-16, cw, 15), 
            GTextOverflowModeWordWrap, 
            GTextAlignmentCenter, 
            NULL); 
    }
    
    
    
    // Fill in the cells with the month days
    int fh;
    int fo;
    GFont font;
    int wknum = 0;
    
    for(i=1;i<=dom;i++){
    
        // New Weeks begin on Sunday
        if(dow > 6){
            dow = 0;
            wknum ++;
        }

        if(invert){
            // Is this today?  If so prep special today style
            if(i==currentTime->tm_mday && offset == 0){
                setInvColors(ctx);
                graphics_fill_rect(
                    ctx,
                    GRect(
                        l+dow*lw+1, 
                        b-(w-wknum)*bh+1, 
                        cw, 
                        ch)
                    ,0
                    ,GCornerNone);
            }
        }

        if(calEvents[i-1]){
        
            font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
            fh = 19;
            fo = 12;
        
        // Normal (non-today) style
        }else{
            font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
            fh = 15;
            fo = 9;
        }
        
        // Draw the day
        graphics_draw_text(
            ctx, 
            intToStr(i),  
            font, 
            GRect(
                cl+dow*lw, 
                b-(-0.5+w-wknum)*bh-fo, 
                cw, 
                fh), 
            GTextOverflowModeWordWrap, 
            GTextAlignmentCenter, 
            NULL); 
        
        // Fix colors if inverted
        if(invert)
            if(offset == 0 && i==currentTime->tm_mday ) setColors(ctx);

        // and on to the next day
        dow++;   
    }
    
    
#if WATCHMODE
    char str[20] = ""; 
    strftime(str, sizeof(str), "%B %d, %Y", currentTime);
#else
    // Build the MONTH YEAR string
    char str[20];
    strcpy (str,months[mon]);
    strcat (str," ");
    strcat (str,intToStr(year));
#endif

    GRect rec = GRect(0, 0, 144, 25);
    if(showtime)
        rec = GRect(0, 40, 144, 25);
    
    // Draw the MONTH/YEAR String
    graphics_draw_text(
        ctx, 
        str,  
        fonts_get_system_font(FONT_KEY_GOTHIC_24), 
        rec,
        GTextOverflowModeWordWrap, 
        GTextAlignmentCenter, 
        NULL);
}

void updateTime(struct tm * t){

    curHour=t->tm_hour;
    curMin=t->tm_min;
    curSec=t->tm_sec;
    
    static char timeText[] = "00:00";
    if(clock_is_24h_style()){
        strftime(timeText, sizeof(timeText), "%H:%M", t);
        if(curHour<10)memmove(timeText, timeText+1, strlen(timeText)); 
    }else{
        strftime(timeText, sizeof(timeText), "%I:%M", t);
        if( (curHour > 0 && curHour<10) || (curHour>12 && curHour<22))memmove(timeText, timeText+1, strlen(timeText)); 
    }
    text_layer_set_text(timeLayer, timeText);
    
    
//    static char dateText[30];
//    strftime(dateText, sizeof(dateText), "%B %d", t);//"%A\n%B %d", t);
//    text_layer_set_text(&dateLayer, dateText);
}

static void get_settings(){
    DictionaryIterator *iter;

    if (app_message_outbox_begin(&iter) != APP_MSG_OK) {
       // app_log(APP_LOG_LEVEL_DEBUG, "calendarApp.c",364,"App MSG Not ok");
        return;
    }    
    if (dict_write_uint8(iter, GET_SETTINGS, ((uint16_t)0)) != DICT_OK) {
     //   app_log(APP_LOG_LEVEL_DEBUG, "calendarApp.c",364,"Dict Not ok");
        return;
    }
    if (app_message_outbox_send() != APP_MSG_OK){
      //  app_log(APP_LOG_LEVEL_DEBUG, "calendarApp.c",364,"Message Not Sent");
    }
}
static void send_cmd(){
        
    time_t now = time(NULL);
    struct tm *currentTime = localtime(&now);
        
    int year = currentTime->tm_year;
    int month = currentTime->tm_mon+offset ;
    
    while(month>11){
        month -= 12;
        year++;
    }
    while(month<0){
        month += 12;
        year--;
    }
    if(year*100+month>0){
        DictionaryIterator *iter;

        if (app_message_outbox_begin(&iter) != APP_MSG_OK) {
           // app_log(APP_LOG_LEVEL_DEBUG, "calendarApp.c",364,"App MSG Not ok");
            return;
        }    
        if (dict_write_uint16(iter, GET_EVENT_DAYS, ((uint16_t)year*100+month)) != DICT_OK) {
         //   app_log(APP_LOG_LEVEL_DEBUG, "calendarApp.c",364,"Dict Not ok");
            return;
        }
        if (app_message_outbox_send() != APP_MSG_OK){
          //  app_log(APP_LOG_LEVEL_DEBUG, "calendarApp.c",364,"Message Not Sent");
        }
    }
}

void processSettings(uint8_t encoded[2]){

    black           = (encoded[0] & (1 << 0)) != 0;
    grid            = (encoded[0] & (1 << 1)) != 0;
    invert          = (encoded[0] & (1 << 2)) != 0;
    showtime        = (encoded[0] & (1 << 3)) != 0;
    start_of_week   = (int) encoded[1];

    if(persist_exists(BLACK_KEY        ))persist_delete(BLACK_KEY);
    if(persist_exists(GRID_KEY         ))persist_delete(GRID_KEY);
    if(persist_exists(INVERT_KEY       ))persist_delete(INVERT_KEY);
    if(persist_exists(SHOWTIME_KEY     ))persist_delete(SHOWTIME_KEY);
    if(persist_exists(START_OF_WEEK_KEY))persist_delete(START_OF_WEEK_KEY);

    persist_write_bool(BLACK_KEY, black);
    persist_write_bool(GRID_KEY, grid);
    persist_write_bool(INVERT_KEY, invert);
    persist_write_bool(SHOWTIME_KEY, showtime);
    persist_write_int(START_OF_WEEK_KEY, start_of_week);
    
    if(black)
        window_set_background_color(window, GColorBlack);
    else
        window_set_background_color(window, GColorWhite);
    
    
    if(showtime){
    
        if(black)
            text_layer_set_text_color(timeLayer, GColorWhite);
        else
            text_layer_set_text_color(timeLayer, GColorBlack);
        time_t now = time(NULL);
        struct tm *currentTime = localtime(&now);
        updateTime(currentTime);
    }
    else{
        text_layer_set_text(timeLayer, "");
    }
    layer_mark_dirty(days_layer);
}
void processEncoded(uint8_t encoded[42]){
    int index;
    for (int byteIndex = 0;  byteIndex < 4; byteIndex++){
        for (int bitIndex = 0;  bitIndex < 8; bitIndex++){
            index = byteIndex*8+bitIndex;
            calEvents[index] = (encoded[byteIndex] & (1 << bitIndex)) != 0;
        }
    }
}
void clearCalEvents(){

    time_t now = time(NULL);
    struct tm *currentTime = localtime(&now);
        
    int year = currentTime->tm_year;
    int month = currentTime->tm_mon+offset ;
    
    while(month>11){
        month -= 12;
        year++;
    }
    while(month<0){
        month += 12;
        year--;
    }
    
    for (int i = 0; i < 31; ++i){
        calEvents[i] = false;
    }
    
    if(year*100+month > 0 && persist_exists(year*100+month)){
        uint8_t encoded[42];
        persist_read_data(year*100+month, encoded, 8);
        processEncoded(encoded);
    }

}

#if !WATCHMODE
static void monthChanged(){

    clearCalEvents();
    send_cmd();
    
    layer_mark_dirty(days_layer);
}
#endif

void my_in_rcv_handler(DictionaryIterator *received, void *context) {
    // incoming message received
    
    uint16_t dta = 0;
    int y = 0;
    int m = 0;
    uint8_t *encoded = 0;
    
    Tuple *tuple = dict_read_first(received);
    while (tuple) {
        switch (tuple->key) {
            case MONTHYEAR_KEY:
                dta = tuple->value->uint16;
                m = dta%100;
                y = (dta-m)/100;
                
                break;
            case EVENT_DAYS_DATA_KEY:
                encoded = tuple->value->data;
                break;
            case SETTINGS_KEY:
                processSettings(tuple->value->data);
                break;
        }
        tuple = dict_read_next(received);
    }
    if(dta>0){
        persist_write_data(dta, encoded, sizeof(encoded));
        time_t now = time(NULL);
        
        struct tm *currentTime = localtime(&now);
        int year = currentTime->tm_year;
        int month = currentTime->tm_mon+offset ;
        
        while(month>11){
            month -= 12;
            year++;
        }
        while(month<0){
            month += 12;
            year--;
        }
        if((m==month && y == year) ){
            processEncoded(encoded);
            layer_mark_dirty(days_layer);
        }else{
            send_cmd();
        }
    }
}


#if !WATCHMODE
void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
    offset--;
    monthChanged();
}
void down_single_click_handler(ClickRecognizerRef recognizer, void *context) {
    offset++;
    monthChanged();
}
void select_single_click_handler(ClickRecognizerRef recognizer, void *context) {
    if(offset != 0){
        offset = 0;
        monthChanged();
    }
}
static void click_config_provider(void* context){
    
    window_single_click_subscribe(          BUTTON_ID_SELECT,  select_single_click_handler);
// Up and Down should be repeating clicks, but due to bug in 2.0 firmware repeating clicks cause watch to crash
  window_single_repeating_click_subscribe(BUTTON_ID_UP  , 100,   up_single_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, down_single_click_handler);
//    window_single_click_subscribe(BUTTON_ID_UP  ,   up_single_click_handler);
//    window_single_click_subscribe(BUTTON_ID_DOWN,   down_single_click_handler);

   
}
#endif

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
    if (showtime && units_changed & MINUTE_UNIT) {
        updateTime(tick_time);  
    }
    if (units_changed & HOUR_UNIT) {
        send_cmd();
    }
    if (units_changed & DAY_UNIT) {
        layer_mark_dirty(days_layer);
    }
}

void init() {
    
    if( persist_exists(BLACK_KEY))          black =         persist_read_bool(BLACK_KEY);
    if( persist_exists(GRID_KEY))           grid =          persist_read_bool(GRID_KEY);
    if( persist_exists(INVERT_KEY))         invert =        persist_read_bool(INVERT_KEY);
    if( persist_exists(SHOWTIME_KEY))       showtime =      persist_read_bool(SHOWTIME_KEY);
    if( persist_exists(START_OF_WEEK_KEY))  start_of_week = persist_read_int(START_OF_WEEK_KEY);
    
    clearCalEvents();
    app_comm_set_sniff_interval(SNIFF_INTERVAL_REDUCED);
    app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
    app_message_register_inbox_received(my_in_rcv_handler);
    
    window = window_create();
    
    window_set_fullscreen(window, true);
#if !WATCHMODE
    window_set_click_config_provider(window, (ClickConfigProvider) click_config_provider);
#endif


    
    if(black)
        window_set_background_color(window, GColorBlack);
    else
        window_set_background_color(window, GColorWhite);

    window_stack_push(window, false);
    
    Layer *window_layer = window_get_root_layer(window);
    
    days_layer = layer_create(layer_get_bounds(window_layer));
    layer_set_update_proc(days_layer, days_layer_update_callback);
    layer_add_child(window_layer, days_layer);


    timeLayer = text_layer_create( GRect(0, -6, 144, 43));
    if(black)
        text_layer_set_text_color(timeLayer, GColorWhite);
    else
        text_layer_set_text_color(timeLayer, GColorBlack);

    text_layer_set_background_color(timeLayer, GColorClear);
    text_layer_set_font(timeLayer, fonts_get_system_font(FONT_KEY_BITHAM_42_MEDIUM_NUMBERS));
    text_layer_set_text_alignment(timeLayer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(timeLayer));
    
    if(showtime){
        time_t now = time(NULL);
        struct tm *currentTime = localtime(&now);
        updateTime(currentTime);
    }

    tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);

    get_settings();
    app_timer_register(500,send_cmd,NULL);
}

static void deinit(void) {

    layer_destroy(days_layer);
    text_layer_destroy(timeLayer);
    tick_timer_service_unsubscribe();
    window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
