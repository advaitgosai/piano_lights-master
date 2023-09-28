#include <Adafruit_NeoPixel.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

#define BUTTON0 34
#define BUTTON1 0
#define BUTTON2 35
#define CHANGE_MODE 0
#define KEY_PRESS 1
#define KEY_RELEASE 2

// set up the LEDs
#define PIN 2
#define NUM_PIXELS 175
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_PIXELS, PIN, NEO_GRB + NEO_KHZ800);
byte led_on_or_off[NUM_PIXELS];
uint32_t led_colors[NUM_PIXELS];

// define different states
#define MAIN_MENU 0
#define CHOOSE_MODE 1
#define PLAYING_SONG 2
Adafruit_SSD1306 lcd(128, 64);
unsigned long press_times[3] = {0, 0, 0};
byte buttons[] = {BUTTON0, BUTTON1, BUTTON2};
int state = MAIN_MENU;
byte debounce_delay = 250;
int selected = 0;
int mode_selected = 0;
boolean changed_color = false;
String mode_array[] = {"Play Song", "Play Game", "Cancel"};
unsigned int num_songs = 6;
String songs[] = {
  "Mozart - Rondo Alla Turca (Turkish March)", 
  "Beethoven - Fur Elise ", 
  "Liszt - La campanella ", 
  "Liszt- Un sospiro", 
  "Twinkle Twinkle Little Star", 
  "Surprise"
};
String colors[] = {
  "Red",
  "Orange",
  "Yellow",
  "Green",
  "Blue",
  "Cyan",
  "Violet",
  "White",
  "Rainbow",
  "Random"
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
#define NUM_COLORS 10
byte curr_color_index = 0;
boolean custom_string_looped_once = false; // used to only loop the string once
int16_t print_string_x_offset = 0;

// below are variables for now_playing screen
int16_t now_playing_song_offset = 0;

uint32_t get_rainbow_color(unsigned char key_num) {
  double percent = key_num / 87.0;
  unsigned int loop_until = percent * 1275;
  unsigned char rgbs[3] = {255, 0, 0};
  int increase_mode = 1;
  for (unsigned int i = 0; i < loop_until; i++) {
    int idx = 0;
    for (int j = 2; j > -1; j--)
      if (rgbs[j] == 255) {
        idx = j;
        break;
      }
    if (increase_mode) {
      rgbs[(idx+1)%3]++;
      if (rgbs[(idx+1)%3] == 255) {
        increase_mode = 0;
      }
    } else {
      int x = 0;
      if (idx-1 < 0) {
        x = 2;
      } else {
        x = idx - 1;
      }
      rgbs[x]--;
      if (rgbs[x] == 0) {
        increase_mode = 1;
      }
    }
  }
  return strip.Color(rgbs[0], rgbs[1], rgbs[2]);
}


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
  strip.show();
  strip.setBrightness(255);
  for (int i = 0; i < NUM_PIXELS; i++) {
    led_on_or_off[i] = 0;
  }
  Serial.begin(38400);
  Serial.setTimeout(0);
  light_animation();
  randomSeed(165);
}

void light_animation() {
  // do the entire animation in 1000 ms
//  long double del = 500.0/NUM_PIXELS;
  for (int i = 0; i < NUM_PIXELS - 3; i++) {
    // sets the previous one to off    
    if (i > 0) strip.setPixelColor(i - 1, 0);
    if (curr_color_index == 8)
      strip.setPixelColor(i, get_rainbow_color((double)(i)/NUM_PIXELS*87));
    else if (curr_color_index == 9)
      strip.setPixelColor(i, strip.Color((byte)random(255), (byte)random(255), (byte)random(255)));
    else
      strip.setPixelColor(i, color_vals[curr_color_index]);
    strip.show();

  }
  strip.setPixelColor(NUM_PIXELS - 1, 0);
  strip.show();
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
  else if (state == CHOOSE_MODE) choose_song_mode();
  else now_playing();
 
  lcd.display();
  if (changed_color) {
    changed_color = false;
    light_animation();
  }
}

void turn_off_leds() {
  for (int i = 0; i < NUM_PIXELS; i++) {
    led_on_or_off[i] = 0;
  }
}

void choose_song_mode() {
  unsigned long current_time = millis();
  for (byte i = 0; i < 3; i++) {
    if (digitalRead(buttons[i]) == 0 && current_time - press_times[i] > debounce_delay) {
      press_times[i] = current_time;
      switch (buttons[i]) {
        case BUTTON0:
          // up;
          mode_selected--;
          if (mode_selected < 0) mode_selected = 2;
          break;
        case BUTTON1:
          // down
          mode_selected = ++mode_selected%(3);
          break;
        case BUTTON2:
          // select the current thing
          if (mode_selected == 2) {
            state = MAIN_MENU;
            custom_string_looped_once = true;
            
          } else {
            // Send Choice to Piano, 0 = Play Song Entirely, 1 = Play Piano Tiles Game
            // {4 header, 0 (play song), song index, choice)}
            byte buf[7] = {100, 101, 102, 103, 0, selected - 1, (byte) mode_selected};
            Serial.write(buf, 7);
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
  
  center_string("Choose Mode", 0, 16);
  String divider = String("-");
  for (byte i=0; i < 20; i++) divider += "-";
  center_string(divider, lcd.getCursorX(), lcd.getCursorY());
  for (byte i = 0; i < 3; i++) {
    // first sort by if selected
    if (mode_selected == i)  {
      // then sort by if it's the cancel case
      if (mode_selected == 2) {
          center_string("Cancel", 0, 56);
          lcd.setCursor(35, 56);
          drawArrow();
      } else customPrintString(mode_array[i]);
    } else {
      if (i == 2) {
          center_string("Cancel", 0, 56);
      } else lcd.println(mode_array[i]); 
    }
  }
  
}

void now_playing() {
  // handle input
  // TODO: Handle input from piano for when it tells you that the song is done
  //      - also handle input for left hand/right hand key pressing
  // probably makes more sense for this to get data about:
  // [100 101 102 103 key_press/key_release OR data about song ending, key_number, what color to set to]
  
  handle_game_mode_piano_input();
  
  unsigned long current_time = millis();
  if (digitalRead(buttons[2]) == 0 && current_time - press_times[2] > debounce_delay) {
    // go back to main menu
    press_times[2] = current_time;
    state = MAIN_MENU;
    custom_string_looped_once = true; 
    // also turn off leds
    turn_off_leds();
    // tell raspberrpy pi to turn off
    byte buf[7] = {100, 101, 102, 103, 1, 0, 0};
    Serial.write(buf, 7);
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
  strip.show();
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

byte incoming[9];
void handle_game_mode_piano_input() {
  if (Serial.available()) {
      byte sets = Serial.available()/9;
      for (int i = 0; i < sets; i++) {
        Serial.readBytes(incoming, 9);
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
          turn_off_leds();
        } else {
          // should not reach here
        }
      }
      update_leds();  
  } 
}

void handle_normal_mode_piano_input() {
  if (Serial.available()) { 
      byte sets = Serial.available()/9;
      for (int i = 0; i < sets; i++) {
        Serial.readBytes(incoming, 9);
        // check header data 
        for (int i = 0; i < 4; i++) {
           if (incoming[i] != i+100) return;
        }
        // key press case, light the corresponding LEDs
        
        if (incoming[4] == KEY_PRESS || incoming[4] == KEY_RELEASE) {
          if (curr_color_index == 8)
            update_key(incoming[5], incoming[4], get_rainbow_color(incoming[5]));
          else if (curr_color_index == 9)
            update_key(incoming[5], incoming[4], strip.Color((byte)random(255), (byte)random(255), (byte)random(255)));  
          else
            update_key(incoming[5], incoming[4], color_vals[curr_color_index]);  
          
        } else if (incoming[4] == CHANGE_MODE) {
          // change mode case, means that the user completed playing the song
          // should not reach here
        } else {
          // should not reach here
        }
      }
  } 
  update_leds();
}

void main_menu() {
  byte page_number = 0;
  if (0 <= selected && selected < 5) page_number = 0;
  else page_number = (selected+3)/8;
  lcd.setCursor(0,page_number*(-64)); // for every page number, set back starting pixel by 64 (one screen)
  
  // manage input
  // TODO: Handle input for when piano key is pressed (should just turn on corresponding LED)
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
              changed_color = true;
          } else {
            // also change the screen
            state = CHOOSE_MODE;
            now_playing_song_offset = 0;
            turn_off_leds();
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
