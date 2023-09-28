#include <Adafruit_NeoPixel.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

#define BUTTON0 34
#define BUTTON1 0
#define BUTTON2 35
#define CHANGE_MODE 0
#define KEY_PRESS 1
#define KEY_RELEASE 2
#define INC_SIZE 9

#define show_noglitch() {delay(1);strip.show();delay(1);strip.show();}
void main_menu();
void handle_game_mode_piano();

// set up the LEDs
#define PIN 2
#define NUM_PIXELS 175
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, PIN, NEO_GRB + NEO_KHZ800);
byte led_on_or_off[NUM_PIXELS];
uint32_t led_colors[NUM_PIXELS];

// define different states
#define MAIN_MENU 0
#define PLAYING_SONG 1
Adafruit_SSD1306 lcd(128, 64);
unsigned long press_times[3] = {0, 0, 0};
byte buttons[] = {BUTTON0, BUTTON1, BUTTON2};
boolean state = MAIN_MENU;
byte debounce_delay = 250;
int selected = 0;
unsigned int num_songs = 8;
String songs[] = {
  "Nocturne Op9 No2", 
  "Rondo Alla Turca", 
  "Ode to Joy", 
  "Heart and Soul", 
  "Twinkle Twinkle Little Star", 
  "Happy Birthday",
  "Test 1",
  "Test 2"
};
String colors[] = {
  "Red",
  "Orange",
  "Yellow",
  "Green",
  "Blue",
  "Cyan",
  "Violet",
  "White"
};
uint32_t color_vals[] = {
  strip.Color(255,0,0),
  strip.Color(255,140,0),
  strip.Color(255,255,0),
  strip.Color(0,255,0),
  strip.Color(0,0,255),
  strip.Color(0,255,255),
  strip.Color(148,0,211),
  strip.Color(255,255,255)
};
#define NUM_COLORS 8
byte curr_color_index = 0;
boolean custom_string_looped_once = false; // used to only loop the string once
int16_t print_string_x_offset = 0;

// below are variables for now_playing screen
int16_t now_playing_song_offset = 0;
int rgb[3] = {0, 0, 0}; // color to use if its in the "game mode"


void setup() {
  pinMode(BUTTON0, INPUT_PULLUP);
  pinMode(BUTTON1, INPUT_PULLUP);
  pinMode(BUTTON2, INPUT_PULLUP);
  lcd.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  lcd.clearDisplay();
  lcd.setTextColor(WHITE);
  lcd.setTextWrap(false);
  lcd.setTextSize(1); // each row is 8 pixels tall
  strip.begin();
  strip.clear();
  show_noglitch()
  strip.setBrightness(255);
  for (int i = 0; i < NUM_PIXELS; i++) {
    led_on_or_off[i] = 0;
  }
  Serial.begin(38400);
  Serial.setTimeout(0);
  light_animation();
}

void light_animation() {
  // do the entire animation in 1000 ms
//  long double del = 500.0/NUM_PIXELS;
  for (int i = 0; i < NUM_PIXELS; i++) {
    // sets the previous one to off
    if (i > 0) strip.setPixelColor(i - 1, 0);
    strip.setPixelColor(i, color_vals[curr_color_index]);
    show_noglitch()
//    delay(del);
  }
  strip.setPixelColor(NUM_PIXELS - 1, 0);
  show_noglitch()
}

void drawArrow() {
  int16_t x = lcd.getCursorX();
  int16_t y = lcd.getCursorY();
  // takes the current position of the cursor and then draws triangle
  // fillTriangle(x0, y0, x1, y1, x2, y2, color)
//  lcd.fillTriangle(lcd.getCursorX(), lcd.getCursorY(), lcd.getCursorX(), lcd.getCursorY() + 6, lcd.getCursorX() + 4, lcd.getCursorY() + 3, WHITE);
  
  // cursor doesn't automatically move over, so do it manually since
  // we want text to show up after
//  lcd.setCursor(lcd.getCursorX() + 10, lcd.getCursorY());
  lcd.drawLine(x, y+3, x+6, y+3, WHITE); // main line
  lcd.drawLine(x+4,y+1, x+6, y+3, WHITE); // top arrow part
  lcd.drawLine(x+4,y+5, x+6, y+3, WHITE); // top arrow part
  lcd.setCursor(lcd.getCursorX() + 11, lcd.getCursorY());
}

void customPrintString(String txt) {
  int16_t  x1, y1;
  uint16_t w, h;
  
  lcd.getTextBounds(txt, 0, 0, &x1, &y1, &w, &h);

  // if length of width is less than 128, doesn't even
  // need loop
  boolean needs_loop = w > 128; 
  
  if (!needs_loop || custom_string_looped_once) {
    drawArrow();
    lcd.println(txt);
  } else {
    // print text starting from offset
    lcd.setCursor(lcd.getCursorX() + print_string_x_offset, lcd.getCursorY());
    lcd.print(txt);
    
    // give illusion of looping    
    for (byte i = 0; i < 5; i++) lcd.print(" "); 
    
    // also check cursor position here to see if 
    // since if it's <= 11, that means that it's about to be done
    // fully looping so we can stop
    if (lcd.getCursorX() <= 11) {
      custom_string_looped_once = true;
    }
    
    lcd.print(txt);
    //
    
    lcd.setCursor(0, lcd.getCursorY());
    lcd.fillRect(0, lcd.getCursorY(), 11, 8, BLACK);
    drawArrow();
    lcd.println();
    print_string_x_offset -= 1;
  }
}

void loop() {
  lcd.clearDisplay();
  if (state == MAIN_MENU) main_menu();
  else now_playing();
  lcd.display();
}


void now_playing() {
  // handle input
  // TODO: Handle input from piano for when it tells you that the song is done
  //      - also handle input for left hand/right hand key pressing
  // probably makes more sense for this to get data about:
  // [100 101 102 103 key_press/key_release OR data about song ending, key_number, what color to set to]
  handle_game_mode_piano();
  
  unsigned long current_time = millis();
  if (digitalRead(buttons[2]) == 0 && current_time - press_times[2] > debounce_delay) {
    // go back to main menu
    press_times[2] = current_time;
    state = MAIN_MENU;
    custom_string_looped_once = true;
    byte buf[] = {100, 101, 102, 103, 1};
    Serial.write(buf, 5);
  }
  // render menu
  center_string("Now Playing:", 0, 16);
  // selected song is selected - 1
  String song = songs[selected-1];
  
  // determine if string should loop or not 
  int16_t  x1, y1;
  uint16_t w, h;
  
  lcd.getTextBounds(song, 0, 0, &x1, &y1, &w, &h);

  // if length of width is less than 128, doesn't even
  // need loop
  boolean needs_loop = w > 128; 
  if (needs_loop) {
    lcd.setCursor(now_playing_song_offset, 32); // sets in position
    lcd.print(song); 
    for (byte i = 0; i < 5; i++) lcd.print(" "); 
    // loops everything back around so it looks like it goes on forever
    if (lcd.getCursorX() == 0) now_playing_song_offset = 0; 
    lcd.print(song); 
    now_playing_song_offset--;  
  } else {
    center_string(song, 0, 32);
  }
  center_string("Cancel", 0, 48);
  lcd.setCursor(35, 48);
  drawArrow();
}

void center_string(String buf, int x, int y)
{
    int16_t x1, y1;
    uint16_t w, h;
    lcd.getTextBounds(buf, x, y, &x1, &y1, &w, &h); //calc width of new string
    lcd.setCursor((128-w)/2, y);
    lcd.println(buf);
}

void update_leds() {
  for (int i = 0; i < NUM_PIXELS; i++) {
    // checks if current led should be on
    strip.setPixelColor(i, led_on_or_off[i] ? led_colors[i] : 0);
  }
  show_noglitch()
}

void update_key(byte key_num, byte press_or_release, uint32_t color) {
  unsigned int led_num = key_num/87.0*NUM_PIXELS; 
  // updates value of on or off in array
  for (int i = -1; i < 2; i++) {
    int l_num = led_num + i;
    if (l_num < 0 || l_num >= NUM_PIXELS) continue;
    if (press_or_release == KEY_RELEASE) led_on_or_off[l_num]--;
    else {
      led_on_or_off[l_num]++;
      led_colors[l_num] = color;
    }
  }
}

byte incoming[INC_SIZE];

void handle_game_mode_piano_input() { // this is actually the same code as the normal mode piano, can 
  if (Serial.available()) {
      int sets = Serial.available()/INC_SIZE;
      Serial.readBytes(incoming, INC_SIZE);
      for (int i = 0; i < sets; i++) {
        // check header data 
        for (int i = 0; i < 4; i++) {
           if (incoming[i] != i+100) return;
        }
        // key press case, light the corresponding LEDs
        if (incoming[4] == KEY_PRESS || incoming[4] == KEY_RELEASE) {
          // incoming[6], [7], and [8] are the RGB values sent from the rpi 
          update_key(incoming[5], incoming[4], strip.Color(incoming[6], incoming[7], incoming[8]));    
        } else if (incoming[4] == CHANGE_MODE) {
          // change mode case, means that the user completed playing the song
          state = MAIN_MENU;
          // update_leds(); // if we don't call update_leds() here, then all the LEDs that
          // want to be updated won't be 
          // actually maybe we don't need it since update_leds gets called in the end anyways
        } else {
          // should not reach here
        }
      }
      update_leds();
    }
  } 

void handle_normal_mode_piano_input() {
  if (Serial.available()) {
    int sets = Serial.available()/INC_SIZE; 
    for (int i = 0; i < sets; i++) {
      Serial.readBytes(incoming, INC_SIZE);
      // check header data 
      for (int i = 0; i < 4; i++) {
         if (incoming[i] != i+100) return;
      }
      // key press case, light the corresponding LEDs
      
      if (incoming[4] == KEY_PRESS || incoming[4] == KEY_RELEASE) {
        update_key(incoming[5], incoming[4], color_vals[curr_color_index]);  
      } else if (incoming[4] == CHANGE_MODE) {
        // change mode case, means that the user completed playing the song
        // should not reach here
      } else {
        // should not reach here
      }
    }
    update_leds();
  } 
}

void main_menu() {
  byte page_number = 0;
  if (0 <= selected && selected < 5) page_number = 0;
  else page_number = (selected+3)/8;
  lcd.setCursor(0,page_number*(-64)); // for every page number, set back starting pixel by 64 (one screen)
  
  // manage input
  handle_normal_mode_piano_input();
  
  unsigned long current_time = millis();
  for (byte i = 0; i < 3; i++) {
    if (digitalRead(buttons[i]) == 0 && current_time - press_times[i] > debounce_delay) {
      press_times[i] = current_time;
      switch (buttons[i]) {
        case BUTTON0:
          // up;
          selected--;
          if (selected < 0) selected = num_songs;
          break;
        case BUTTON1:
          // down
          selected = ++selected%(num_songs+1);
          break;
        case BUTTON2:
          // select the current thing
          if (selected == 0) {
              // change the color index
              curr_color_index = (curr_color_index+1)%NUM_COLORS;
              light_animation();
          } else {
            // TODO: Send data to the piano about what song is being played now
            byte buf[6] = {100, 101, 102, 103, 0, (byte) selected - 1};
            Serial.write(buf, 6);
            // also change the screen
            state = PLAYING_SONG;
            now_playing_song_offset = 0;
          }
          break;
      }
      custom_string_looped_once = false;
      print_string_x_offset = 0;
    }
  }

  String curr_text = String("Curr Color: ") + colors[curr_color_index];
  center_string(curr_text, 0, lcd.getCursorY());
  String next_text = String("Set to ") + colors[(curr_color_index+1)%NUM_COLORS];
  if (selected == 0) {
    // prints the centered text first and then adds the arrow
    int16_t x1, y1;
    uint16_t w, h;
    lcd.getTextBounds(next_text, lcd.getCursorX(), lcd.getCursorY(), &x1, &y1, &w, &h); //calc width of new string
    lcd.setCursor((128-w)/2, lcd.getCursorY());
    lcd.print(next_text);

    lcd.setCursor((128-w)/2 - 11, lcd.getCursorY());
    drawArrow();
    lcd.println();
  } else {
    center_string(next_text, lcd.getCursorX(), lcd.getCursorY());
  }

  String divider = String("-");
  for (byte i=0; i < 20; i++) divider += "-";
  center_string(divider, lcd.getCursorX(), lcd.getCursorY());
  center_string("Songs", lcd.getCursorX(), lcd.getCursorY());

  
  for (byte i = 0; i < num_songs; i++) {
    if (selected == i + 1)  {
      customPrintString(songs[i]);
    } else {
      lcd.println(songs[i]); 
    }
  }

}

/*  TODOS:
 *  - Now Playing Screen:
 *    - Needs:
 *      - the screen should go away if the song is already over, so should try to account for that too
 * 
 * flow chart for processing midi notes in python file:
 * - If Left hand:
 *    - key_press AND key_release:
 *      - press/release the note (sound) AND turn on/off the corresponding LEDs (given the color)
 * - If right hand:     
 *    - key_press:
 *      - DON'T play the note (the user should play it)
 *      - Send a command saying that it should be green (or whatever right hand color)
 *      - BLOCK and wait for the user
 *      - IF the user plays the right key, set the color to yellow (or whatever color for hold is)
 *    - key_release
 *      - DON"T play the note
 *      - Send a command saying that the corresponding LEDs should be off (would've been in the intermediate state of holding)
 *      - go onto the next note
 */
