// Application status
#define APP_ERROR   -1000
// For previous state at setup time
#define APP_START  -2
// App is displaying splash screen while initializing.
#define APP_SPLASH -1
// App is waiting for sensors or buttons changes : Valve is closed
// displaying current passing volume, desired volume, total volume, flowrate
// push button may open valve after APP_CONFIRM mode
#define APP_WAITING 0
// App is running water thru valve : valve is opened
// displaying current passing volume, desired volume, total volume, flowrate
// push button may interrupt to close valve and return to APP_WAITING mode
#define APP_RUNNING 1
// App is in setting mode, valve is closed
// displying 'setting mode' on first line and desired volume to adjust on second line
// turing the rotary encoder adjust disered volume of water,
// push button may set adjusted volume of water and return to APP_WAITING mode
#define APP_SETTING 2
// App is in confirmation mode, valve is closed
// displaying a confirmation message on first line and  yes | no choice on second line.
// push button may set desired volume and return to APP_WAITING mode
#define APP_CONFIRM 3
// App is in confirmation mode, valve is closed,
// displaying a confirmation message on first line and  [run] [set] [x]
// push button may set desired volume and return to APP_RUNNING, APP_SETTING, or APP_WAITING mode
#define APP_OPTIONS 4
// Reseting app, erasing all stored values
#define APP_RESET 5

// Choices on Options screen
//#define CHOICE_UNDEFINED -100
#define CHOICE_CANCEL   0
#define CHOICE_RUNNING  1
#define CHOICE_SETTING  2
#define CHOICE_RESET    3

// Application variables
int app_status;
int app_previous_status;
String app_error = "";

/*************************************************
  Application Setup (include to general setup)
**************************************************/
void application_setup() {
  app_previous_status = APP_START;
  app_status = APP_SPLASH;
  button_was_pushed = true; // Simulate the first button push to enter in splash screen.
  //app_choice = CHOICE_UNDEFINED;
}

/*************************************************
  Application Reset Erasing all stored values
**************************************************/
void application_reset() {
  encoder_setup();
  flowmeter_reset();
  flowmeter_setup();
  application_setup();
}

/*************************************************
   Set current application state, taking the
   previous step in account
 **************************************************/
void application_set_current_state() {
  // See where we were before this event
  switch (app_previous_status) {
    case APP_RESET:
      application_reset();
    // no break here, to go to case APP_START

    case APP_START:
      Serial.print("APP_START -> ");
      // Starting application
      app_status = APP_SPLASH;
      break;

    case APP_SPLASH:
      Serial.print("APP_SPLASH -> ");
      app_status = APP_WAITING;
      break;

    case APP_WAITING:
      Serial.print("APP_WAITING -> ");
      app_status = APP_OPTIONS;
      break;

    case APP_RUNNING:
      Serial.print("APP_RUNNING -> ");
      app_status = APP_WAITING;
      break;

    case APP_SETTING:
      Serial.print("APP_SETTING -> ");
      app_status = APP_WAITING;
      break;

    case APP_OPTIONS:
      Serial.print("APP_OPTIONS -> ");
      break;

    default:
      Serial.print("APP_ERROR -> ");
      // see if we need to reset something ?
      app_error = "No known state for LCD ?";
      app_status = APP_ERROR;
      break;
  }
  app_previous_status = app_status;
}

/*************************************************
   Handling current state
   Displaying screens

   /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\
   On fait des IF car l'instruction Switch
   est boguée lorsqu'il y a plus d'un certains nb de lignes
   elle déconne et ne prend pas en compte les derniers CASE.

   Switch instruction seems to be buggy when it has
   a certain number of lines in it.
   Last cases are not taken in account !
   /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\ /!\
 **************************************************/
void application_display() {

  // App is waiting for sensors or buttons changes : Valve is closed
  if ( app_status == APP_SPLASH ) {
    Serial.println(" APP_SPLASH");
    valve.close();
    // displaying splash screen
    lcd.clear();
    lcd_splash_screen();
    lcd_print();
    lcd_change_backlight();
  }

  // Waiting for a button push to start running water
  if ( app_status == APP_WAITING ) {
    Serial.println(" APP_WAITING");
    valve.close();
    flowmeter_calculate_pct_of_target_liters();
    // Background color is blue, dodgerBlue.
    lcd_setbacklight(30, 144, 255);
    lcd.clear();
    lcd_waiting_mode(0.0, flowmeter_total_liters, app_pct_target_liters, flowmeter_liters);
    lcd_print();
  }

  // Displays configuration menu
  if ( app_status == APP_OPTIONS ) {
    Serial.println(" APP_OPTIONS");
    valve.close();
    lcd.clear();
    lcd_options_mode();
    lcd_print();
  }

  // Displays screen to se target liters
  if ( app_status == APP_SETTING ) {
    Serial.println(" APP_SETTING");
    valve.close();
    lcd.clear();
    lcd_setting_mode(String(app_target_liters));
    lcd_print();
  }

  // Running water thru valve and counting via flowmeter
  if ( app_status == APP_RUNNING ) {
    Serial.println(" APP_RUNNING");
    valve.open();
    lcd.clear();
    lcd_running_mode(0.0, flowmeter_total_liters, app_pct_target_liters, flowmeter_liters);
    lcd_print();
  }

}

/*************************************************
   applications screens controller
 **************************************************/
void handle_application_screens() {
  if ( button_was_pushed ) {
    // Set state from previous state to current state
    application_set_current_state();
    // Displays new screen state
    application_display();
    button_was_pushed = false;
  }
}

/*************************************************
   applications menu choices controller
 **************************************************/
void handle_application_choices() {
  if ( button_was_turned ) {
    // Applicative choice
    if (app_previous_status == APP_OPTIONS) {

      set_screen_choice(encoderPosCount, 4);
      // We were in options mode, so see which option was choosen.
      switch (screen_choice) {
        case CHOICE_CANCEL:
          Serial.println("CHOICE_CANCEL");
          // init flow meter variables
          //flw_pulses_old = flw_pulses;
          app_status = APP_WAITING; // Canceling any action, go to waiting state
          break;
        case CHOICE_RUNNING:
          Serial.println("CHOICE_RUNNING");
          app_status = APP_RUNNING; // Go to running mode, valve opened
          break;
        case CHOICE_SETTING:
          Serial.println("CHOICE_SETTING");
          app_status = APP_SETTING;
          break;
        case CHOICE_RESET:
          Serial.println("CHOICE_RESET");
          app_status = APP_RESET;
          break;
        default:
          break;
      }
      // Let's do the maj of the screen
      lcd.clear();
      lcd_options_mode();
      lcd_print();
      delay(300); // debounce
    } else {

      if (app_status == APP_SETTING) {
        flowmeter_calculate_target_liters();
        lcd.clear();
        lcd_setting_mode(String(app_target_liters));
        lcd_print();
      }
    }

    button_was_turned = false;
  }

}

/*************************************************
   applications menu choices controller
 **************************************************/
void handle_application_set_target_liters() {
  if ( button_was_turned ) {
    // Setting targt liters
    if (app_status == APP_SETTING) {
      flowmeter_calculate_target_liters();
      lcd.clear();
      lcd_setting_mode(String(app_target_liters));
      lcd_print();
    }
    button_was_turned = false;
  }
}

/*************************************************
   applications flometer reading
 **************************************************/
void handle_application_flometer() {
  if (app_status == APP_RUNNING) {
    if (flowmeter_was_turning) {
      // displaying current passing volume, desired volume, total volume, flowrate
      float frac = (flw_rate - int(flw_rate)) * 10;

      flowmeter_calculate_pct_of_target_liters();
      
      if (valve.status() == HIGH) {
        // Set backlight to a various color that say it's open.
        // The color changes on pct increase.
        lcd_adjust_backlight(app_pct_target_liters);
      }

      // see if we got 100% of target?
      if (app_pct_target_liters >= 100) {
        app_pct_target_liters = 100;
        // Forcing waiting State, this stat closes valve
        app_status = APP_WAITING;
        // Simulate a state change
        button_was_pushed = true;
      }

      lcd.clear();
      lcd_running_mode(frac, flowmeter_total_liters, app_pct_target_liters, flowmeter_liters);
      lcd_print();

      flowmeter_was_turning = false;
    }
  }
}
