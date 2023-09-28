#include <Adafruit_NeoPixel.h>
#define show_noglitch() {delay(1);strip.show();delay(1);strip.show();}
#define PIN 2
#define NUM_LEDS 175
#define BAUD_RATE 38400
#include <Adafruit_SSD1306.h>

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);
int leds[NUM_LEDS];
int left_hand_color[3] = {0, 0, 255};
int right_hand_color[3] = {255, 255, 255};
int led_divider = 71;
Adafruit_SSD1306 lcd(128, 64);

void setup() {
  // put your setup code here, to run once:
  strip.begin();
  strip.setBrightness(10);
  strip.clear();
  show_noglitch();
  Serial.begin(BAUD_RATE);
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = 0;
  }
  Serial.setTimeout(0);
    lcd.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  lcd.clearDisplay();
  lcd.setTextColor(WHITE);
  lcd.setTextWrap(false);
  lcd.setTextSize(1); // each row is 8 pixels tall
}

void update_leds() {
  for (int i = 0; i < NUM_LEDS; i++) {
    // does two checks: if the led should be on and if so what color it should be
    boolean right = led_divider <= i;
    byte r = (right ? right_hand_color : left_hand_color)[0];
    byte g = (right ? right_hand_color : left_hand_color)[1];
    byte b = (right ? right_hand_color : left_hand_color)[2];
    strip.setPixelColor(i, leds[i] ? strip.Color(r, g, b) : 0);
  }
  show_noglitch();
}

void test_print(int x) {
  lcd.clearDisplay();
  lcd.setCursor(0, 0);
  lcd.print(x);
  lcd.display();
}

byte data[7];
void loop() {
  if (Serial.available()) {
    // data_in = [4 header, 1 or 0 (this is about key press or cancelling the song), ...]
    //         = [4 header, 1/2 (key_press/release), KEY_NUM, r, g, b]
    //         = [4 header, 0 (change current mode)]
    // data_out = [4 header, 1 or 0 (this is about cancelling song or starting to play a song)]
    //          = [4 header, 1 (cancel the song)]
    //          = [4 header, 0, song_index_to_play]
    int sets = Serial.available() / 7;
    for (int i = 0; i < sets; i++) {
      Serial.readBytes(data, 7);
      for (int i = 0; i < 4; i++) {
         if (data[i] != i+100) return;
      }
      // change LED case
      if (data[4] == 0) {
        byte key_num = data[5]/87.0*NUM_LEDS;
        int val = data[6] ? 1 : -1;
        leds[key_num] += val;
  
        if (key_num > 0) leds[key_num - 1] += val;
        if (key_num < NUM_LEDS - 1) leds[key_num + 1] += val;
      } else if (data[4] == 1){
        // change divider case
        int note_divider = data[5];
        // need to turn the note into corresponding LED number to divide
        led_divider = (int) ((note_divider/87.0) * NUM_LEDS);
      }
    }
    update_leds();
  }
}
