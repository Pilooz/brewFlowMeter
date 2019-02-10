// LCD variables
rgb_lcd lcd;
String screen_line1, screen_line2;
const int lcd_colorR = 255;
const int lcd_colorG = 0;
const int lcd_colorB = 0;
int lcd_brightness = 100;
// Reset menu
const String menus1_reset[] = {
  "Reset values ?  ",
  "Reset values ?  "
};
const String menus2_reset[] = {
  "   [No]  Yes    ",
  "    No   [Yes]  "
};
// Options Menu
const String menus1_opt[] = {
  "[cancel]   run  ",
  " cancel   [run] ",
  " cancel    run  ",
  " cancel    run  "
};
const String menus2_opt[] = {
  " set     reset  ",
  " set     reset  ",
  "[set]    reset  ",
  " set    [reset] "
};

int screen_choice = 0;

/*************************************************
   Setting screen_choice index with encoder
   current and last position
 **************************************************/
void set_screen_choice(uint8_t currPos, uint8_t nb_choices) {
  screen_choice += currPos;
  if (screen_choice < 0 || screen_choice > nb_choices - 1) {
    screen_choice = 0;
  }
}

/*************************************************
   Printing the 2 lines screen to lcd
 **************************************************/
void lcd_setup() {
  lcd.begin(16, 2);
  lcd.setRGB(lcd_colorR, lcd_colorG, lcd_colorB);
}

/*************************************************
   Printing the 2 lines screen to lcd
 **************************************************/
void lcd_print() {
  lcd.setCursor(0, 0);
  lcd.print(screen_line1);
  lcd.setCursor(0, 1);
  lcd.print(screen_line2);
}

/*************************************************
   set backlight color on lcd
 **************************************************/
void lcd_setbacklight(uint8_t r, uint8_t g, uint8_t b) {
  lcd.setRGB(r, g, b);
}

/*************************************************
   adusting backlight color on lcd
   dealing with pct of volume flown
 **************************************************/
void lcd_adjust_backlight(int pct) {
  // mapping pct between 0 and 255
  int p = (int)(255 * pct / 100);
  lcd_setbacklight(255 - p, p, 0);
}

/*************************************************
   Displaying a splash screen at startup
 **************************************************/
void lcd_splash_screen() {
  screen_line1 = " BrewFlowMeter ";
  screen_line2 = " v" + String(APP_VERSION) + " by Pilooz ";
}

/**************************************************
   Displaying data on lcd
   Step : APP_RESET
   displaying :
    Reset values ?
    [No] [Yes]
 **************************************************/
void lcd_reset_mode() {
  // background color Orange
  lcd_setbacklight(255, 50, 0);
  screen_line1 = menus1_reset[screen_choice];
  screen_line2 = menus2_reset[screen_choice];
}

/*************************************************
  Displaying data on lcd
  Step : APP_OPTIONS
  displaying :
  Choice ?
  [Run] [Set] [x] >
**************************************************/
void lcd_options_mode() {
  // background color Orange
  lcd_setbacklight(255, 50, 0);
  screen_line1 = menus1_opt[screen_choice];
  screen_line2 = menus2_opt[screen_choice];
}

/*************************************************
   Displaying data on lcd
   Step : APP_SETTING
   Turning the rotary enc CW increments by 0.1 L
   turninc CCW decrements by 0.1 L
   displaying :
     Target volume ?
     0.0 L
 **************************************************/
void lcd_setting_mode(String nb_liters) {
  // background color Orange
  lcd_setbacklight(255, 165, 0);
  screen_line1 = "Target volume ? ";
  screen_line2 = nb_liters + " L ";
}

/*************************************************
  Displaying data on lcd
  Step : APP_WAITING
  displaying :
  flowrate L/s     total volume L
  percent flow %   current passed volume / desired volume
**************************************************/
void lcd_waiting_mode(float flow_rate, float total_liters, float pct, float flow_liters) {
  //char rate[4];
  //dtostrf(flow_rate, 4, 1, rate);
  //screen_line1 = String(rate) + "L/min " + String(total_liters) + " L";
  screen_line1 = "Total     " + String(total_liters) + " L";
  screen_line2 = String(int(pct)) + "% " + String(flow_liters) + "/" + String(app_target_liters) + " L";
}

/*************************************************
   Displaying data on lcd
   Step : APP_RUNNING
   displaying :
    flowrate L/s     total volume L
    percent flow %   current passed volume / desired volume
 **************************************************/
void lcd_running_mode(float flow_rate, float total_liters, float pct, float flow_liters) {
  lcd_waiting_mode(flow_rate, total_liters, pct, flow_liters);
}

/*************************************************
   Setup for serial
 **************************************************/
void lcd_message(String msg) {
  screen_line1 = "Error !";
  screen_line2 = msg;
}

/*************************************************
   Testing all the created screens
 **************************************************/
void lcd_test_screens() {
  // Print a message to the LCD.
  lcd.clear();
  lcd_splash_screen();
  lcd_print();
  delay(1000);

  // Testing LCD (All screens)
  // Options Screen
  lcd.clear();
  for (int x = 0; x <= 3; x++) {
    screen_choice = x;
    lcd_options_mode();
    lcd_print();
    delay(1000);
  }

  // Reset Screen
  lcd.clear();
  for (int x = 0; x <= 1; x++) {
    screen_choice = x;
    lcd_reset_mode();
    lcd_print();
    delay(1000);
  }

  // Settings Screen
  lcd.clear();
  for (int x = 0; x < 100; x++) {
    //app_target_liters = encoderPos * ENC_STEP;
    lcd_setting_mode(String(x) + "." + String(x));
    lcd_print();
    delay(100);
  }
  delay(1000);

  // Error Message Screen
  lcd.clear();
  lcd_message("This is an error");
  lcd_print();
  for (int x = 0; x < 100; x++) {
    lcd_adjust_backlight(x);
    lcd_message("Adjusting BG color");
    delay(200);
  }

  // Waiting Screen
  lcd.clear();
  lcd_waiting_mode(1.0, 10.5, 23.4, 56.7);
  delay(1000);

  // Running Screen
  lcd.clear();
  lcd_running_mode(1.0, 10.5, 23.4, 56.7);
  delay(1000);
}
