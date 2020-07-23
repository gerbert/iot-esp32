#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Ticker.h>
#include <IPAddress.h>
#include <sys/time.h>

#define ARRAY_SZ(x)     (size_t)(sizeof(x) / sizeof(x[0]))

#define SCREEN_WIDTH    128         //! <OLED display width, in pixels
#define SCREEN_HEIGHT   32          //! <OLED display height, in pixels
#define OLED_RESET      4           //! <Reset pin # (or -1 if sharing Arduino reset pin)
#define IIC_OLED_ADDR   0x3C
#define IIC_SDA_GPIO    5
#define IIC_SCL_GPIO    4
#define IIC_CLK_HZ      400000UL

typedef struct {
    IPAddress ip;
    struct tm *time;
    float temperature;
    float humidity;
    uint16_t pressure;
    float voltage;
} disp_data;

static const char *sta_ssid = "%some_ssid%";
static const char *sta_psk = "%some_psk%";
static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
static disp_data data;
static time_t t_now;
static Ticker t_sync_tck;
static Ticker t_wait_wifi_ack_tck;

static void __sync_cb(void) {
    timeval tv;
    time_t now;

    gettimeofday(&tv, NULL);
    t_now = time(&tv.tv_sec);

    data.time = localtime(&t_now);
    data.ip = WiFi.localIP();
}

static void __wait_for_wifi(void) {
    //! <Wait while WiFi gets connected
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
    }

    //! <Set NTP server parameters
    configTime(7200, 3600, "time.nist.gov", "pool.ntp.org");
    //! <Call sync function once
    __sync_cb();
    //! <Create a task for continuous synchronization once in a minute
    t_sync_tck.attach(60, __sync_cb);

    //! <Detach from the function
    t_wait_wifi_ack_tck.detach();
}

static void __display_update(void)
{
    //! <Clear display buffer
    display.clearDisplay();

    //! <Default: font size 1:1
    display.setTextSize(1);
    //! <Default: white letters on black screen
    display.setTextColor(WHITE);

    //! <Print battery voltage
    display.setCursor(0, 0);
    display.printf("%1.3fV", data.voltage);

    //! <Print time
    display.setCursor(96, 0);
    if (data.time != NULL) {
        display.printf("%02d:%02d", data.time->tm_hour, data.time->tm_min);
    } else {
        display.printf("00:00");
    }

    //! <Print temperature
    display.setCursor(0, 8);
    display.printf("Out: %3.1f\370C", data.temperature);

    //! <Print humidity
    display.setCursor(90, 8);
    display.printf("%3.1f%%", data.humidity);

    //! <Display pressure
    display.setCursor(0, 16);
    display.printf("Pressure (in.Hg):");
    display.setCursor(102,16);
    display.printf("%4d", data.pressure);

    //! <Print IP address
    display.setCursor(0, 24);
    display.printf("%d.%d.%d.%d", data.ip[0], data.ip[1], data.ip[2], data.ip[3]);

    //! <Print signal strength
    display.setCursor(102, 24);
    display.printf("%4d", WiFi.RSSI());

    //! <Update the display
    display.display();
}

void setup() {
    //! <Initialise IIC interface
    Wire.begin(IIC_SDA_GPIO, IIC_SCL_GPIO, IIC_CLK_HZ);
    //! <SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if (!display.begin(SSD1306_SWITCHCAPVCC, IIC_OLED_ADDR)) {
        for(;;); //! <Don't proceed, loop forever
    }

    //! <Set up WiFi client
    WiFi.mode(WIFI_STA);
    WiFi.begin(sta_ssid, sta_psk);

    memset(&data, 0, sizeof(disp_data));

    //! <Dummy data
    data.humidity = 27.4;
    data.pressure = 730;
    data.temperature = -12.4;
    data.voltage = 3.347;

    //! <Scheduled calls
    t_wait_wifi_ack_tck.once(1, __wait_for_wifi);
}

void loop() {
    __display_update();
}
