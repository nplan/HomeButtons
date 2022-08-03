#include "display.h"

const int WIDTH = 128;
const int HEIGHT = 296;

const uint16_t W = WIDTH / 2;

const uint16_t H1 = round(HEIGHT/12.);
const uint16_t H2 = round(HEIGHT/12. + HEIGHT/6.);
const uint16_t H3 = round(HEIGHT/12. + 2*HEIGHT/6.);
const uint16_t H4 = round(HEIGHT/12. + 3*HEIGHT/6.);
const uint16_t H5 = round(HEIGHT/12. + 4*HEIGHT/6.);
const uint16_t H6 = round(HEIGHT/12. + 5*HEIGHT/6.);

const uint16_t num_fonts = 4;
const GFXfont* fonts[] = {&FreeSansBold24pt7b, &FreeSansBold18pt7b, &FreeSansBold12pt7b, &FreeSansBold9pt7b};
const uint16_t num_buttons = 6;
const uint16_t heights[] = {H1, H2, H3, H4, H5, H6};

const uint16_t h_padding = 5;

#define GxEPD2_DISPLAY_CLASS GxEPD2_BW
#define GxEPD2_DRIVER_CLASS GxEPD2_290_T94_V2
#define MAX_DISPLAY_BUFFER_SIZE 65536ul // e.g.
#define MAX_HEIGHT(EPD) (EPD::HEIGHT <= MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8) ? EPD::HEIGHT : MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8))

GxEPD2_DISPLAY_CLASS<GxEPD2_DRIVER_CLASS, MAX_HEIGHT(GxEPD2_DRIVER_CLASS)> *display;

namespace eink {

    void begin() {
        display = new GxEPD2_DISPLAY_CLASS<GxEPD2_DRIVER_CLASS, MAX_HEIGHT(GxEPD2_DRIVER_CLASS)>(GxEPD2_DRIVER_CLASS(/*CS=*/ HW.EINK_CS, /*DC=*/ HW.EINK_DC, /*RST=*/ HW.EINK_RST, /*BUSY=*/ HW.EINK_BUSY));
        display->init();
    }
    
    void hibernate() {
        display->hibernate();
    }

    void display_string(const char* string) {
        display->setRotation(0);
        display->setTextColor(GxEPD_BLACK);
        display->setTextWrap(true);
        display->setFont(&FreeSans9pt7b);
        display->setPartialWindow(0, 0, display->width(), display->height());

        display->firstPage();
        do {
            display->setCursor(0, 30);
            display->fillScreen(GxEPD_WHITE);
            display->print(string);
        }
        while (display->nextPage());
    }

    void display_error(const char* string) {
        display->setRotation(0);
        display->setTextColor(GxEPD_BLACK);
        display->setTextWrap(false);
        display->setFont(&FreeSansBold9pt7b);
        display->setPartialWindow(0, 0, display->width(), display->height());

        display->firstPage();
        do {
            display->setCursor(0, 0);
            int16_t x, y;
            uint16_t w, h;
            display->getTextBounds("ERROR", 0, 0, &x, &y, &w, &h);
            display->setCursor(WIDTH/2 - w/2, 20);
            display->print("ERROR");

            display->setFont(&FreeSans9pt7b);
            display->setTextWrap(true);
            display->setCursor(0, 60);
            display->print(string);
        }
        while (display->nextPage());
    }

    void display_buttons(const char* button_1_text,
                        const char* button_2_text,
                        const char* button_3_text,
                        const char* button_4_text,
                        const char* button_5_text,
                        const char* button_6_text) {

        const char* texts[] = {button_1_text, button_2_text, button_3_text,
                                button_4_text, button_5_text, button_6_text};

        display->setRotation(0);
        display->setTextColor(GxEPD_BLACK);
        display->setTextWrap(false);
        display->setFullWindow();

        display->firstPage();
        do {
            display->fillScreen(GxEPD_WHITE);
            display->setFont(&FreeSansBold18pt7b);
            
            int16_t x, y;
            uint16_t w, h;
            
            // Loop through buttons
            for (uint16_t i=0; i<num_buttons; i++) {
            display->getTextBounds(texts[i], 0, 0, &x, &y, &w, &h);
            int16_t w_pos, h_pos;
            if (i % 2 == 0) {
                w_pos = -x + h_padding;
            }
            else {
                w_pos = WIDTH - w - x - h_padding;
            }
            h_pos = heights[i] - y - h/2;
            display->setCursor(w_pos, h_pos);
            display->print(texts[i]);
            }
        }
        while (display->nextPage());
    }

    void display_ap_config_screen(String ssid, String password) {
        String contents = String("WIFI:T:WPA;S:") + ssid + ";P:" + password + ";;";

        uint8_t version = 6; // 41x41px
        QRCode qrcode;
        uint8_t qrcodeData[qrcode_getBufferSize(version)];
        qrcode_initText(&qrcode, qrcodeData, version, ECC_HIGH, contents.c_str());

        display->setRotation(0);
        display->setTextColor(GxEPD_BLACK);
        display->setTextWrap(false);
        display->setFont(&FreeSans9pt7b);

        display->firstPage();
        do {
            display->fillScreen(GxEPD_WHITE);

            int16_t x, y;
            uint16_t w, h;
            display->getTextBounds("Scan:", 0, 0, &x, &y, &w, &h);
            display->setCursor(WIDTH/2 - w/2, 20);
            display->print("Scan:");

            uint16_t qr_x = 23;
            uint16_t qr_y = 35;
            for (uint8_t y = 0; y < qrcode.size; y++) {
            // Each horizontal module
            for (uint8_t x = 0; x < qrcode.size; x++) {

                // Display each module
                if (qrcode_getModule(&qrcode, x, y)) {
                display->drawRect(qr_x+x*2, qr_y+y*2, 2, 2, GxEPD_BLACK);
                }
            }
            }
            display->setFont(&FreeSansBold9pt7b);
            display->getTextBounds("------ or ------", 0, 0, &x, &y, &w, &h);
            display->setCursor(WIDTH/2 - w/2, 153);
            display->print("------ or ------");

            display->setFont(&FreeSans9pt7b);
            display->getTextBounds("Connect to:", 0, 0, &x, &y, &w, &h);
            display->setCursor(WIDTH/2 - w/2, 190);
            display->print("Connect to:");

            display->setCursor(0, 220);
            display->print("WiFi:");
            display->setFont(&FreeSansBold9pt7b);
            display->setCursor(0, 235);
            display->print(ssid.c_str());

            display->setFont(&FreeSans9pt7b);
            display->setCursor(0, 260);
            display->print("Password:");
            display->setFont(&FreeSansBold9pt7b);
            display->setCursor(0, 275);
            display->print(password.c_str());

        }
        while (display->nextPage());
    }

    void display_web_config_screen(String ip) {
        String contents = String("http://") + ip; 

        uint8_t version = 6; // 41x41px
        QRCode qrcode;
        uint8_t qrcodeData[qrcode_getBufferSize(version)];
        qrcode_initText(&qrcode, qrcodeData, version, ECC_HIGH, contents.c_str());

        display->setRotation(0);
        display->setTextColor(GxEPD_BLACK);
        display->setTextWrap(false);
        display->setFont(&FreeSans9pt7b);

        display->firstPage();
        do {
            display->fillScreen(GxEPD_WHITE);

            int16_t x, y;
            uint16_t w, h;
            display->getTextBounds("Scan:", 0, 0, &x, &y, &w, &h);
            display->setCursor(WIDTH/2 - w/2, 20);
            display->print("Scan:");

            uint16_t qr_x = 23;
            uint16_t qr_y = 35;
            for (uint8_t y = 0; y < qrcode.size; y++) {
            // Each horizontal module
            for (uint8_t x = 0; x < qrcode.size; x++) {

                // Display each module
                if (qrcode_getModule(&qrcode, x, y)) {
                display->drawRect(qr_x+x*2, qr_y+y*2, 2, 2, GxEPD_BLACK);
                }
            }
            }
            display->setFont(&FreeSansBold9pt7b);
            display->getTextBounds("------ or ------", 0, 0, &x, &y, &w, &h);
            display->setCursor(WIDTH/2 - w/2, 153);
            display->print("------ or ------");

            display->setFont(&FreeSans9pt7b);
            display->getTextBounds("Go to:", 0, 0, &x, &y, &w, &h);
            display->setCursor(WIDTH/2 - w/2, 200);
            display->print("Go to:");

            display->setFont(&FreeSansBold9pt7b);
            display->setCursor(0, 240);
            display->print("http://");
            display->setCursor(0, 260);
            display->print(ip.c_str());

        }
        while (display->nextPage());
    }

    void display_wifi_connected_screen() {
        display->setRotation(0);
        display->setTextColor(GxEPD_BLACK);
        display->setTextWrap(true);
        display->setFont(&FreeSansBold9pt7b);
        display->setPartialWindow(0, 0, display->width(), display->height());

        display->firstPage();
        do {
            display->setCursor(0, 0);
            display->fillScreen(GxEPD_WHITE);

            int16_t x, y;
            uint16_t w, h;
            display->getTextBounds("WIFI", 0, 0, &x, &y, &w, &h);
            display->setCursor(WIDTH/2 - w/2, 120);
            display->print("WIFI");

            display->getTextBounds("CONNECTED", 0, 0, &x, &y, &w, &h);
            display->setCursor(WIDTH/2 - w/2, 150);
            display->print("CONNECTED");

            display->getTextBounds(":)", 0, 0, &x, &y, &w, &h);
            display->setCursor(WIDTH/2 - w/2, 180);
            display->print(":)");
        }
        while (display->nextPage());
    }

    void display_wifi_not_connected_screen() {
        display->setRotation(0);
        display->setTextColor(GxEPD_BLACK);
        display->setTextWrap(true);
        display->setFont(&FreeSansBold9pt7b);
        display->setPartialWindow(0, 0, display->width(), display->height());

        display->firstPage();
        do {
            display->setCursor(0, 0);
            display->fillScreen(GxEPD_WHITE);

            int16_t x, y;
            uint16_t w, h;
            display->getTextBounds("WIFI", 0, 0, &x, &y, &w, &h);
            display->setCursor(WIDTH/2 - w/2, 90);
            display->print("WIFI");

            display->getTextBounds("NOT", 0, 0, &x, &y, &w, &h);
            display->setCursor(WIDTH/2 - w/2, 120);
            display->print("NOT");

            display->getTextBounds("CONNECTED", 0, 0, &x, &y, &w, &h);
            display->setCursor(WIDTH/2 - w/2, 150);
            display->print("CONNECTED");
        }
        while (display->nextPage());
    }

    void display_setup_complete_screen() {
        display->setRotation(0);
        display->setTextColor(GxEPD_BLACK);
        display->setTextWrap(true);
        display->setFont(&FreeSansBold9pt7b);
        display->setPartialWindow(0, 0, display->width(), display->height());

        display->firstPage();
        do {
            display->setCursor(0, 0);
            display->fillScreen(GxEPD_WHITE);

            int16_t x, y;
            uint16_t w, h;
            display->getTextBounds("SETUP", 0, 0, &x, &y, &w, &h);
            display->setCursor(WIDTH/2 - w/2, 120);
            display->print("SETUP");

            display->getTextBounds("COMPLETE", 0, 0, &x, &y, &w, &h);
            display->setCursor(WIDTH/2 - w/2, 150);
            display->print("COMPLETE");

            display->getTextBounds(":)", 0, 0, &x, &y, &w, &h);
            display->setCursor(WIDTH/2 - w/2, 180);
            display->print(":)");
        }
        while (display->nextPage());
    }

    void display_please_recharge_soon_screen() {
        display->setRotation(0);
        display->setTextColor(GxEPD_BLACK);
        display->setTextWrap(true);
        display->setFont(&FreeSansBold9pt7b);
        display->setPartialWindow(0, 0, display->width(), display->height());

        display->firstPage();
        do {
            display->setCursor(0, 0);
            display->fillScreen(GxEPD_WHITE);

            int16_t x, y;
            uint16_t w, h;
            display->getTextBounds("PLEASE", 0, 0, &x, &y, &w, &h);
            display->setCursor(WIDTH/2 - w/2, 90);
            display->print("PLEASE");

            display->getTextBounds("RECHARGE", 0, 0, &x, &y, &w, &h);
            display->setCursor(WIDTH/2 - w/2, 120);
            display->print("RECHARGE");

            display->getTextBounds("BATTERY", 0, 0, &x, &y, &w, &h);
            display->setCursor(WIDTH/2 - w/2, 150);
            display->print("BATTERY");

            display->getTextBounds("SOON", 0, 0, &x, &y, &w, &h);
            display->setCursor(WIDTH/2 - w/2, 180);
            display->print("SOON");
        }
        while (display->nextPage());
    }

    void display_turned_off_please_recharge_screen() {
        display->setRotation(0);
        display->setTextColor(GxEPD_BLACK);
        display->setTextWrap(false);
        display->setFont(&FreeSansBold9pt7b);
        display->setPartialWindow(0, 0, display->width(), display->height());

        display->firstPage();
        do {
            display->setCursor(0, 0);
            display->fillScreen(GxEPD_WHITE);

            int16_t x, y;
            uint16_t w, h;
            display->getTextBounds("TURNED OFF", 0, 0, &x, &y, &w, &h);
            display->setCursor(WIDTH/2 - w/2, 60);
            display->print("TURNED OFF");

            display->getTextBounds("PLEASE", 0, 0, &x, &y, &w, &h);
            display->setCursor(WIDTH/2 - w/2, 120);
            display->print("PLEASE");

            display->getTextBounds("RECHARGE", 0, 0, &x, &y, &w, &h);
            display->setCursor(WIDTH/2 - w/2, 150);
            display->print("RECHARGE");

            display->getTextBounds("BATTERY", 0, 0, &x, &y, &w, &h);
            display->setCursor(WIDTH/2 - w/2, 180);
            display->print("BATTERY");
        }
        while (display->nextPage());
    }

    void display_please_complete_setup_screen(const char * uid) {
        display->setRotation(0);
        display->setTextColor(GxEPD_BLACK);
        display->setTextWrap(false);

        display->firstPage();
        do {
            display->setCursor(0, 0);
            display->fillScreen(GxEPD_WHITE);

            int16_t x, y;
            uint16_t w, h;
            display->setFont(&FreeSans18pt7b);
            display->getTextBounds("Home", 0, 0, &x, &y, &w, &h);
            display->setCursor(WIDTH/2 - w/2, 60);
            display->print("Home");
            display->getTextBounds("Buttons", 0, 0, &x, &y, &w, &h);
            display->setCursor(WIDTH/2 - w/2 - 3, 100);
            display->print("Buttons");

            display->setTextWrap(true);
            display->setFont(&FreeSansBold9pt7b);
            display->setCursor(0, 160);
            display->print("Press\nany button\nto start\nsetup");

            display->setFont(&FreeMono9pt7b);
            display->setCursor(0, 275);
            display->print(uid);
        }
        while (display->nextPage());
    }

    void display_unboxing_screen(const char * uid) {
        display->setRotation(0);
        display->setTextColor(GxEPD_BLACK);
        display->setTextWrap(false);

        display->firstPage();
        do {
            display->setCursor(0, 0);
            display->fillScreen(GxEPD_WHITE);

            int16_t x, y;
            uint16_t w, h;
            display->setFont(&FreeSans18pt7b);
            display->getTextBounds("Home", 0, 0, &x, &y, &w, &h);
            display->setCursor(WIDTH/2 - w/2, 60);
            display->print("Home");
            display->getTextBounds("Buttons", 0, 0, &x, &y, &w, &h);
            display->setCursor(WIDTH/2 - w/2 - 3, 100);
            display->print("Buttons");

            display->setTextWrap(true);
            display->setFont(&FreeSansBold9pt7b);
            display->setCursor(0, 160);
            display->print("Please\nconnect charger\nto wake\ndevice");

            display->setFont(&FreeMono9pt7b);
            display->setCursor(0, 275);
            display->print(uid);
        }
        while (display->nextPage());
    }

    void display_info_screen(float temp, float humid, uint8_t batt) {
        display->setRotation(0);
        display->setTextColor(GxEPD_BLACK);
        display->setTextWrap(false);

        display->firstPage();
        do {
            display->setCursor(0, 0);
            display->fillScreen(GxEPD_WHITE);

            int16_t x, y;
            uint16_t w, h;
            String text;

            text = "-- T --";
            text = "- Temp -";
            display->setFont(&FreeMono9pt7b);
            display->getTextBounds(text, 0, 0, &x, &y, &w, &h);
            display->setCursor(WIDTH/2 - w/2, 30);
            display->print(text);

            text = String(temp, 1) + String(" C");
            display->setFont(&FreeSansBold18pt7b);
            display->getTextBounds(text, 0, 0, &x, &y, &w, &h);
            display->setCursor(WIDTH/2 - w/2 - 2, 70);
            display->print(text);

            text = "-- H --";
            text = "- Humd -";
            display->setFont(&FreeMono9pt7b);
            display->getTextBounds(text, 0, 0, &x, &y, &w, &h);
            display->setCursor(WIDTH/2 - w/2, 129);
            display->print(text);

            text = String(humid, 0) + String(" %");
            display->setFont(&FreeSansBold18pt7b);
            display->getTextBounds(text, 0, 0, &x, &y, &w, &h);
            display->setCursor(WIDTH/2 - w/2 - 2, 169);
            display->print(text);

            text = "-- B --";
            text = "- Batt -";
            display->setFont(&FreeMono9pt7b);
            display->getTextBounds(text, 0, 0, &x, &y, &w, &h);
            display->setCursor(WIDTH/2 - w/2, 228);
            display->print(text);

            text = String(batt) + String(" %");
            display->setFont(&FreeSansBold18pt7b);
            display->getTextBounds(text, 0, 0, &x, &y, &w, &h);
            display->setCursor(WIDTH/2 - w/2 - 2, 268);
            display->print(text);

        }
        while (display->nextPage());
    }

    void display_test_screen() {
        display->setRotation(0);
        display->setTextColor(GxEPD_BLACK);
        display->setTextWrap(true);
        display->setFont(&FreeSans9pt7b);

        display->firstPage();
        do {
            display->setCursor(0, 20);
            display->fillScreen(GxEPD_WHITE);
            display->print(R"(TESTESTESTESTESTESTESTESTESTESTESTEST
                            ESTESTESTESTESTESTESTESTESTESTESTEST
                            ESTESTESTESTESTESTESTESTESTESTESTEST
                            ESTESTESTESTESTESTESTESTESTESTESTEST)");
        }
        while (display->nextPage());
        }

        void display_white_screen() {
        display->firstPage();
        do {
            display->fillScreen(GxEPD_WHITE);
        }
        while (display->nextPage());
        }

        void display_black_screen() {
        display->firstPage();
        do {
            display->fillScreen(GxEPD_BLACK);
        }
        while (display->nextPage());
    }

} // end namespace