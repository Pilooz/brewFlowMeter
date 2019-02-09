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

// Choices on Options screen
#define CHOICE_UNDEFINED -100
#define CHOICE_CANCEL   0
#define CHOICE_RUNNING  1
#define CHOICE_SETTING  2
#define CHOICE_RESET    3

// Application variables
int app_status;
int app_previous_status;
int app_choice;
String app_error = "";


/*************************************************
   Set current application state, taking the
   previous step in account
 **************************************************/
void application_set_current_state() {
  // See where we were before this event
  switch (app_previous_status) {
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
      if (app_choice != CHOICE_UNDEFINED) {
        app_status = app_choice;
      }
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
 **************************************************/
void application_display() {
  float total_liters;
  float liters;

  switch (app_status) {
    case APP_SPLASH: // App is waiting for sensors or buttons changes : Valve is closed
      Serial.println(" APP_SPLASH");
      valve.close();
      // displaying splash screen
      lcd.clear();
      lcd_splash_screen();
      lcd_print();
      lcd_change_backlight();
      break;

    case APP_WAITING: // App is running water thru valve : valve is closed
      Serial.println(" APP_WAITING");
      valve.close();
      // Background color is blue, dodgerBlue.
      lcd_setbacklight(30, 144, 255);
      lcd.clear();
      lcd_waiting_mode(0.0, total_liters, 0.0, liters);
      lcd_print();
      break;

    case APP_RUNNING: // App is waiting for sensors or buttons changes : Valve is opened
      Serial.println(" APP_RUNNING");
      valve.open();
      // displaying current passing volume, desired volume, total volume, flowrate
      //flowmeter_read();
      //float frac = (flw_rate - int(flw_rate)) * 10;

      float pct = 0;
//      if (app_target_liters > 0) {
//        pct = (int)(100 * liters / app_target_liters);
//      }
//
//      if (valve.status() == HIGH) {
//        // Set backlight to a various color that say it's open.
//        // The color changes on pct increase.
//        lcd_adjust_backlight(pct);
//      }
//
//      // see if we got 99% of target?
//      if (pct >= 99) {
//        pct = 100;
//        // Forcing waiting State, this stat closes valve
//        app_status = APP_WAITING;
//      }


      lcd.clear();
      lcd_running_mode(0.0, flowmeter_total_liters, pct, flowmeter_liters);
      lcd_print();
      break;

    case APP_SETTING: // App is in setting mode, valve is closed
      Serial.println(" APP_SETTING");
      valve.close();
      // Calculate target liters
      flowmeter_calculate_target_liters();
      lcd.clear();
      lcd_setting_mode(String(app_target_liters));
      lcd_print();
      break;

    case APP_OPTIONS: // App is in confirmation mode, valve is closed
      Serial.println(" APP_OPTIONS");
      valve.close();
      lcd.clear();
      lcd_options_mode();
      lcd_print();
      break;

    default:
      Serial.println(" APP_ERROR");
      // Close the valve by security
      valve.close();
      // see if we need to reset something ? treating with errors
      lcd.clear();
      lcd_message(app_error);
      lcd_print();
      app_status = APP_ERROR;
      break;
  }
}

/*************************************************
   applications screens controller
 **************************************************/
void handle_application_screens() {
  if ( button_was_pushed ) {
    application_set_current_state();
    application_display();
    button_was_pushed = false;
  }
}

/*************************************************
   applications choices controller
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
          app_choice = APP_WAITING; // Canceling any action, go to waiting state
          break;
        case CHOICE_RUNNING:
          Serial.println("CHOICE_RUNNING");
          app_choice = APP_RUNNING; // Go to running mode, valve opened
          break;
        case CHOICE_SETTING:
          Serial.println("CHOICE_SETTING");
          app_choice = APP_SETTING;
          break;
        case CHOICE_RESET:
          Serial.println("CHOICE_RESET");
          //flowmeter_reset();
          app_choice = APP_SPLASH;
          //lcd_splash_screen();
          break;
        default:
          break;
      }
      // Let's do the maj of the screen
      lcd.clear();
      lcd_options_mode();
      lcd_print();
      //delay(300); // debounce
    }
    button_was_turned = false;
  }

}

/*************************************************
  Application Setup (include to general setup)
**************************************************/
void application_setup() {
  app_previous_status = APP_START;
  app_status = APP_SPLASH;
  button_was_pushed = true; // Simulate the first button push to enter in splash screen.
}
