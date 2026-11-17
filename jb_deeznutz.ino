/*
 * PS5 WebKit exploit auto-navigator
 *
 * Waveshare ESP32-S3-Zero — USB HID keyboard
 * Continuously pings PS5 at 192.168.0.118
 * - Port 9021 open → already jailbroken, do nothing
 * - Ping responds but 9021 closed → run navigation chain
 * - After nav chain, wait 30s for 9021
 * - If 9021 still not up → reboot PS5 via REST mode
 *
 * USB-OTG pins: GPIO19 (D-), GPIO20 (D+)
 * WiFi: for TCP ping checks + log server
 * RGB LED on GPIO21: status indicator
 *
 * LED colors:
 *   RED     — no WiFi
 *   BLUE    — monitoring (PS5 off or not responding)
 *   YELLOW  — PS5 up, navigating to exploit
 *   MAGENTA — waiting for JB payload listener
 *   GREEN   — jailbroken (port 9021 open)
 */

#include <WiFi.h>
#include "USB.h"
#include "USBHIDKeyboard.h"

#define LED_PIN      21
#define PS5_IP       IPAddress(192, 168, 0, 125)
#define JB_PORT      9021
#define CHECK_INTERVAL_MS  5000
#define JB_WAIT_TIMEOUT_MS 30000
#define NAV_COOLDOWN_MS    20000

const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";

USBHIDKeyboard kb;

// --- TCP log server (port 9999) ---
#define LOG_PORT 9999
#define MAX_LOG_CLIENTS 3
WiFiServer logServer(LOG_PORT);
WiFiClient logClients[MAX_LOG_CLIENTS];

void log_accept() {
    WiFiClient c = logServer.available();
    if (c) {
        for (int i = 0; i < MAX_LOG_CLIENTS; i++) {
            if (!logClients[i] || !logClients[i].connected()) {
                logClients[i] = c;
                c.println("=== PS5 HID Auto-Navigator log ===");
                return;
            }
        }
        c.println("max clients");
        c.stop();
    }
}

void logln(const char* msg) {
    Serial.println(msg);
    for (int i = 0; i < MAX_LOG_CLIENTS; i++) {
        if (logClients[i] && logClients[i].connected()) {
            logClients[i].println(msg);
        }
    }
}

void logf(const char* fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    logln(buf);
}

enum State {
    STATE_NO_WIFI,
    STATE_MONITORING,
    STATE_NAVIGATING,
    STATE_WAITING_JB,
    STATE_JAILBROKEN
};

State state = STATE_NO_WIFI;
unsigned long last_check = 0;
unsigned long nav_start = 0;
unsigned long last_nav_attempt = 0;
bool led_dirty = true;

// --- LED ---

void set_led(uint8_t g, uint8_t r, uint8_t b) {
    neopixelWrite(LED_PIN, r,g,b);
}

void update_led() {
    switch (state) {
        case STATE_NO_WIFI:    set_led(0, 80, 0);    break; // red
        case STATE_MONITORING: set_led(0, 0, 80);    break; // blue
        case STATE_NAVIGATING: set_led(40, 80, 0);   break; // yellow
        case STATE_WAITING_JB: set_led(0, 60, 60);   break; // magenta
        case STATE_JAILBROKEN: set_led(80, 0, 0);    break; // green
    }
    led_dirty = false;
}

// --- Network checks ---

bool ping_host(IPAddress ip) {
    WiFiClient c;
    c.setTimeout(1000);
    bool up = c.connect(ip, 80);
    if (up) {
        c.stop();
        return true;
    }
    unsigned long t0 = millis();
    c.setTimeout(500);
    c.connect(ip, 9999);
    unsigned long elapsed = millis() - t0;
    c.stop();
    return elapsed < 300;
}

bool check_port(IPAddress ip, uint16_t port) {
    WiFiClient c;
    c.setTimeout(2000);
    bool open = c.connect(ip, port);
    if (open) c.stop();
    return open;
}

// --- HID keyboard helpers ---

void press_key(uint8_t key, uint16_t hold_ms = 100) {
    kb.press(key);
    delay(hold_ms);
    kb.release(key);
    delay(150);
}

void press_enter(uint16_t post_delay = 1000) {
    press_key(KEY_RETURN, 100);
    delay(post_delay);
}

void press_up(uint16_t post_delay = 300) {
    press_key(KEY_UP_ARROW, 100);
    delay(post_delay);
}

void press_down(uint16_t post_delay = 300) {
    press_key(KEY_DOWN_ARROW, 100);
    delay(post_delay);
}

void press_right(uint16_t post_delay = 300) {
    press_key(KEY_RIGHT_ARROW, 100);
    delay(post_delay);
}

void press_left(uint16_t post_delay = 300) {
    press_key(KEY_LEFT_ARROW, 100);
    delay(post_delay);
}

void press_esc(uint16_t post_delay = 500) {
    press_key(KEY_ESC, 100);
    delay(post_delay);
}

void press_back(uint16_t post_delay = 300) {
    press_key(KEY_BACKSPACE, 100);
    delay(post_delay);
}

// --- Navigation chain ---
// up, right x3, enter, up x2, enter, right, enter, enter

void navigate_to_exploit() {
    logln(">>> Starting navigation chain");
    state = STATE_NAVIGATING;
    led_dirty = true;

    // ESC to dismiss anything / go back to known state
    press_esc(200);
    press_esc(200);
    press_esc(200);

    // Enter
    press_up(200);
    press_right(200);
    press_enter(200);
    press_enter(200);
    press_enter(3500);
    press_back();

    logln(">>> Navigation chain complete");
}

void reboot_ps5() {
    logln(">>> Sending reboot sequence");

    for(int i = 0; i < 10; i++){
        press_esc(50);
    }

    logln(">>> No keyboard reboot — will retry nav on next cycle");
}

// --- Main ---

void setup() {
    Serial.begin(115200);
    unsigned long t0 = millis();
    while (!Serial && millis() - t0 < 3000) delay(10);
    Serial.println("\n=== PS5 HID Auto-Navigator ===");

    USB.begin();
    kb.begin();
    Serial.println("USB HID keyboard initialized");

    WiFi.mode(WIFI_STA);
    WiFi.config(IPAddress(192,168,0,123), IPAddress(192,168,0,1), IPAddress(255,255,255,0), IPAddress(192,168,0,1));
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("WiFi connecting");
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 40) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        state = STATE_MONITORING;
        Serial.print("\nWiFi OK, IP: ");
        Serial.println(WiFi.localIP());
        logServer.begin();
        Serial.println("Log server on port 9999");
    } else {
        state = STATE_NO_WIFI;
        Serial.println("\nWiFi FAILED");
    }
    led_dirty = true;
}

void loop() {
    if (led_dirty) update_led();
    log_accept();

    if (WiFi.status() != WL_CONNECTED) {
        if (state != STATE_NO_WIFI) {
            state = STATE_NO_WIFI;
            led_dirty = true;
            logln("WiFi lost");
        }
        static unsigned long last_reconnect = 0;
        if (millis() - last_reconnect > 5000) {
            WiFi.reconnect();
            last_reconnect = millis();
        }
        delay(100);
        return;
    }
    if (state == STATE_NO_WIFI) {
        state = STATE_MONITORING;
        led_dirty = true;
        logln("WiFi restored");
    }

    unsigned long now = millis();

    switch (state) {
        case STATE_MONITORING: {
            if (now - last_check < CHECK_INTERVAL_MS) break;
            last_check = now;

            if (check_port(PS5_IP, JB_PORT)) {
                logln("Port 9021 OPEN -- already jailbroken");
                state = STATE_JAILBROKEN;
                led_dirty = true;
            } else if (ping_host(PS5_IP)) {
                logln("PS5 up, not jailbroken");
                if (now - last_nav_attempt > NAV_COOLDOWN_MS) {
                    last_nav_attempt = now;
                    navigate_to_exploit();
                    nav_start = millis();
                    state = STATE_WAITING_JB;
                    led_dirty = true;
                } else {
                    logln("  (cooldown, waiting)");
                }
            } else {
                logln("PS5 not responding");
            }
            break;
        }

        case STATE_JAILBROKEN: {
            if (now - last_check < CHECK_INTERVAL_MS) break;
            last_check = now;
            if (!check_port(PS5_IP, JB_PORT)) {
                logln("Port 9021 closed -- lost JB, back to monitoring");
                state = STATE_MONITORING;
                led_dirty = true;
            }
            break;
        }

        case STATE_WAITING_JB: {
            if (now - last_check < 3000) break;
            last_check = now;

            if (check_port(PS5_IP, JB_PORT)) {
                logln("JB SUCCESS -- port 9021 open!");
                state = STATE_JAILBROKEN;
                led_dirty = true;
            } else if (now - nav_start > JB_WAIT_TIMEOUT_MS) {
                logln("JB TIMEOUT -- 30s elapsed, no port 9021");
                reboot_ps5();
                state = STATE_MONITORING;
                led_dirty = true;
            } else {
                logf("  Waiting for JB... %lus remaining", (JB_WAIT_TIMEOUT_MS - (now - nav_start)) / 1000);
            }
            break;
        }

        default:
            break;
    }

    delay(100);
}
