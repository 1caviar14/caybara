#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>
#include <math.h>

// ---- Display constants ----
const uint16_t RED     = 0xF800;
const uint16_t GREEN   = 0x07E0;
const uint16_t BLUE    = 0x001F;
const uint16_t CYAN    = 0x07FF;
const uint16_t MAGENTA = 0xF81F;
const uint16_t YELLOW  = 0xFFE0;
const uint16_t WHITE   = 0xFFFF;
const uint16_t GRAY    = 0x520A;
const uint16_t BLACK   = 0x0000;

const uint16_t DISPLAY_WIDTH   = 320;
const uint16_t DISPLAY_HEIGHT  = 480;

// ---- Touch constants ----
const uint16_t PRESSURE_LEFT = 10;
const uint16_t PRESSURE_RIGHT = 1200;

const int XP = 8;
const int XM = A2;
const int YP = A3;
const int YM = 9;

// ✅ Your calibrated values
const int XBEGIN = 186;
const int XEND   = 974;
const int YBEGIN = 963;
const int YEND   = 205;

// ---- App constants ----
const uint16_t CANVAS_X = 10;
const uint16_t CANVAS_Y = 10;

const uint16_t CANVAS_W = DISPLAY_WIDTH - (2 * CANVAS_X);
const uint16_t CANVAS_H = (DISPLAY_HEIGHT / 4) * 3 - (2 * CANVAS_Y);

const uint16_t PAINT_RADIUS = 12;
const uint16_t PAINT_OFFSET_X = 40;
const uint16_t PAINT_OFFSET_Y = CANVAS_Y + CANVAS_H + 30;

const uint16_t PAINT_COORS[][3] = {
    {0, 0, RED},    {35, 0, GREEN},     {70, 0, BLUE},
    {0, 35, CYAN},  {35, 35, MAGENTA},  {70, 35, YELLOW},
    {0, 70, WHITE}, {35, 70, GRAY},     {70, 70, BLACK}
};

const uint16_t THICKNESS_OPTIONS[] = {3, 5, 7, 9};
const uint16_t THICKNESS_OPTION_COORS[][2] = {
    {160, PAINT_OFFSET_Y}, 
    {195, PAINT_OFFSET_Y}, 
    {230, PAINT_OFFSET_Y}, 
    {265, PAINT_OFFSET_Y}
};

// ---- Global state ----
MCUFRIEND_kbv tft;
TouchScreen ts(XP, YP, XM, YM, 300);

uint16_t pen_color = WHITE;
uint16_t thickness_id = 1;

// ---- Utility functions ----
bool in_range(uint16_t value, uint16_t lo, uint16_t hi) {
    return (lo <= value) && (value <= hi);
}

uint32_t distance(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1) {
    uint32_t dx = (x0 > x1) ? (x0 - x1) : (x1 - x0);
    uint32_t dy = (y0 > y1) ? (y0 - y1) : (y1 - y0);
    return sqrt(dx * dx + dy * dy);
}

void to_display_mode() {
    pinMode(XM, OUTPUT);
    pinMode(XP, OUTPUT);
    pinMode(YM, OUTPUT);
    pinMode(YP, OUTPUT);
}

void convert_touch_coors(uint16_t tx, uint16_t ty, uint16_t *xptr, uint16_t *yptr) {
    tx = constrain(tx, XBEGIN, XEND);
    ty = constrain(ty, YBEGIN, YEND);

    tx = map(tx, XBEGIN, XEND, 0, DISPLAY_WIDTH - 1);
    ty = map(ty, YBEGIN, YEND, DISPLAY_HEIGHT - 1, 0);

    *xptr = tx;
    *yptr = ty;
}

void get_touch_coors(uint16_t *xptr, uint16_t *yptr) {
    TSPoint p;
    for (;;) {
        p = ts.getPoint();
        if (in_range(p.z, PRESSURE_LEFT, PRESSURE_RIGHT)) break;
    }
    convert_touch_coors(p.x, p.y, xptr, yptr);
    to_display_mode();
}

// ---- Drawing widgets ----
void draw_canvas() {
    tft.drawRect(CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, WHITE);
}

void draw_color_selector() {
    uint16_t x, y, c;
    for (uint16_t i = 0; i < 9; ++i) {
        x = PAINT_COORS[i][0] + PAINT_OFFSET_X;
        y = PAINT_COORS[i][1] + PAINT_OFFSET_Y;
        c = PAINT_COORS[i][2];

        tft.fillCircle(x, y, PAINT_RADIUS, c);
        tft.drawCircle(x, y, PAINT_RADIUS + 3, WHITE);
    }
}

void draw_size_selector() {
    uint16_t x, y, t;
    for (uint16_t i = 0; i < 4; ++i) {
        x = THICKNESS_OPTION_COORS[i][0];
        y = THICKNESS_OPTION_COORS[i][1];
        t = THICKNESS_OPTIONS[i];

        tft.fillCircle(x, y, t, pen_color);
        if (i == thickness_id)
            tft.drawCircle(x, y, t + 3, WHITE);
        else
            tft.drawCircle(x, y, t + 3, BLACK);
    }
}

// ---- Update widgets ----
void update_canvas(uint16_t x, uint16_t y) {
    uint16_t t = THICKNESS_OPTIONS[thickness_id];
    if (in_range(x, CANVAS_X + t + 2, CANVAS_X + CANVAS_W - t - 2)
        && in_range(y, CANVAS_Y + t + 2, CANVAS_Y + CANVAS_H - t - 2)) {
        tft.fillCircle(x, y, t, pen_color);
    }
}

void update_color_selection(uint16_t x, uint16_t y) {
    for (uint16_t i = 0; i < 9; ++i) {
        uint32_t x0 = PAINT_COORS[i][0] + PAINT_OFFSET_X;
        uint32_t y0 = PAINT_COORS[i][1] + PAINT_OFFSET_Y;
        if (distance(x0, y0, x, y) <= PAINT_RADIUS) {
            pen_color = PAINT_COORS[i][2];
            draw_size_selector();
            break;
        }
    }
}

void update_size_selection(uint16_t x, uint16_t y) {
    for (uint16_t i = 0; i < 4; ++i) {
        uint32_t x0 = THICKNESS_OPTION_COORS[i][0];
        uint32_t y0 = THICKNESS_OPTION_COORS[i][1];
        if (distance(x0, y0, x, y) <= THICKNESS_OPTIONS[i]) {
            thickness_id = i;
            draw_size_selector();
            break;
        }
    }
}

// ---- Main ----
void setup() {
    Serial.begin(115200);
    tft.begin(0x9486); // For ILI9486, adjust if necessary
    tft.setRotation(0);
    tft.fillScreen(BLACK);

    draw_canvas();
    draw_color_selector();
    draw_size_selector();
}

void loop() {
    uint16_t x, y;
    get_touch_coors(&x, &y);

    update_canvas(x, y);
    update_color_selection(x, y);
    update_size_selection(x, y);

    delay(5);
}
