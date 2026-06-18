#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <EEPROM.h>

#define PIN_FUN 3

ISR(WDT_vect) {}

#if MOOD_INDICATOR
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Color;
Color colors[] = {
    { HIGH, LOW, LOW },
    { LOW, HIGH, LOW },
    { LOW, LOW, HIGH },
    { HIGH, HIGH, LOW },
    { LOW, HIGH, HIGH },
    { HIGH, LOW, HIGH },
    { HIGH, HIGH, HIGH },
    { HIGH, HIGH, HIGH }, // flashlight mode
};

uint8_t color;
#define COLOR_LEN (sizeof(colors) / sizeof(colors[0]))
#define LAST_COLOR (COLOR_LEN - 1) // flashlight mode

#define PIN_R 0
#define PIN_G 1
#define PIN_B 4
#elif TAMAGOTCHI
uint8_t hunger;
uint8_t fun;
uint8_t xp;
uint8_t lvl;

uint8_t counter;

static void load_tmg() {
    hunger = EEPROM.read(0);
    fun = EEPROM.read(1);
    xp = EEPROM.read(2);
    lvl = EEPROM.read(3);
}
static void save_tmg() {
    EEPROM.write(0, hunger);
    EEPROM.write(1, fun);
    EEPROM.write(2, xp);
    EEPROM.write(3, lvl);
}
#elif WEATHER_STATION
#include <TinyBMP280.h>
tbmp::TinyBMP180 bmp;
#endif

#if (TAMAGOTCHI || WEATHER_STATION)
#define SSD1306 0x3c
#include <Tiny4kOLED.h>
#endif

void sleep(bool is_long, bool wait = true, uint8_t long_to = WDTO_2S) {
    cli();
    wdt_reset();
    MCUSR &= ~(1 << WDRF);
    // WDTCR |= (1 << WDCE) | (1 << WDE);
    if(wait) {
        WDTCR = (1 << WDCE) | (1 << WDE);
        WDTCR = (1 << WDIE) | (is_long ? long_to : WDTO_120MS);
    }
    sei();

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();

    sleep_cpu();
    sleep_disable();

    wdt_disable();
}

void setup() {
#if TAMAGOTCHI
    wdt_disable();
#endif
    pinMode(PIN_FUN, INPUT_PULLUP);
    if(!digitalRead(PIN_FUN)) while(true) delay(100);

#if (TAMAGOTCHI || WEATHER_STATION)
    oled.begin(128, 64, sizeof(tiny4koled_init_128x64br), tiny4koled_init_128x64br);
    oled.clear();
    oled.on();
    oled.setFont(FONT8X16);
#endif

#if MOOD_INDICATOR
    pinMode(PIN_R, OUTPUT);
    pinMode(PIN_G, OUTPUT);
    pinMode(PIN_B, OUTPUT);

    color = EEPROM.read(0);
    if(color >= COLOR_LEN) color = 0;
    EEPROM.write(0, color + 1);
#endif

#if TAMAGOTCHI
    load_tmg();
    if(hunger > 120) {
        hunger = 0;
        fun = 0;
        xp = 0;
        lvl = 0;
    }
    if(hunger != 0) hunger--;
    save_tmg();

    oled.println(" \\O/");
    oled.println( " -O- ");
    oled.println( " /o\\");
    oled.setFont(FONT6X8);
    oled.print("HUN=");
    oled.print(hunger);
    oled.print(" FUN=");
    oled.println(fun);
    oled.print("XP=");
    oled.print(xp);
    oled.print(" LVL=");
    oled.print(lvl);

    for(uint16_t i = 0; i < 1000; i++) {
        delay(10);
        if(!digitalRead(PIN_FUN)) {
            if(fun != 20) fun++;
            xp++;
            if(xp == 20) {
                xp = 0;
                lvl++;
            }
            if(hunger != 20) hunger++;
            save_tmg();
            while(!digitalRead(PIN_FUN));
            wdt_enable(WDTO_15MS); // will this work? no idea tbh
            while(true);
        }
    }
    oled.clear();
    oled.off();
#elif WEATHER_STATION
    bmp.begin();
    float t = bmp.readTemperature();
    oled.print(t);
    oled.println(" C");
#endif
}
void loop() {
#if TAMAGOTCHI
    sleep(true, true, WDTO_8S);
    if(counter == 255) {
        if(fun != 0) fun--;
        if(hunger != 20) hunger++;
        save_tmg();
        counter = 0;
    }
    counter++;
#elif WEATHER_STATION
    delay(10000);
    oled.clear();
    oled.off();
    sleep(false, false);
#endif

#if MOOD_INDICATOR
    digitalWrite(PIN_R, colors[color].r);
    digitalWrite(PIN_G, colors[color].g);
    digitalWrite(PIN_B, colors[color].b);

    if(color == LAST_COLOR) sleep(false, false);

    sleep(false);

    digitalWrite(PIN_R, 0);
    digitalWrite(PIN_G, 0);
    digitalWrite(PIN_B, 0);

    sleep(true);
#endif
}