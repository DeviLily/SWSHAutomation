#include <SwitchControlLibrary.h>
#include "TM1637.h"

#define S_INTV 50
#define M_INTV 150
#define L_INTV 950

#define BTN1 3
#define BTN2 2
#define BTN3 0
#define BTN4 1
#define DEBOUNCE_DELAY 20

#define CLK A2
#define DIO A3

#define CHAR_I 0x10
#define CHAR_L 0x11
#define CHAR_N 0x12
#define CHAR_T 0x13
#define CHAR_Y 0x14
#define BLANK 0x7f

volatile uint8_t btn_stat;
volatile uint8_t last_btn_stat;
volatile unsigned long last_debounce_time[4] = {0, 0, 0, 0};

TM1637 tm1637(CLK, DIO);
int8_t num_disp[] = {0x00, 0x00, 0x00, 0x00};

const uint8_t month_day[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
uint16_t year;
uint8_t month, day, is_leap_y;

int8_t mode, refresh, cur_pos;
int32_t day_cnt;

void Btn1Pressed();
void Btn2Pressed();
void Btn3Pressed();
void Btn4Pressed();

void DispNDigit(int8_t, int8_t, int32_t);

void MoveCursor(Hat hat);
void PressA();
void PressB();
void PressX();
void PressHome();

void GameToDate();
void DateToGame();
void DatePlusOne();
uint8_t IsLeapYear();
void DatePlusN(int32_t);

void setup() {
  // Default Date
  year = 2020;
  month = 1;
  day = 1;

  btn_stat = 0x00;
  last_btn_stat = 0x00;
  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);
  pinMode(BTN3, INPUT_PULLUP);
  pinMode(BTN4, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BTN1), Btn1Pressed, CHANGE);
  attachInterrupt(digitalPinToInterrupt(BTN2), Btn2Pressed, CHANGE);
  attachInterrupt(digitalPinToInterrupt(BTN3), Btn3Pressed, CHANGE);
  attachInterrupt(digitalPinToInterrupt(BTN4), Btn4Pressed, CHANGE);
  
  tm1637.set();
  tm1637.init();

  SwitchControlLibrary();

  mode = refresh = 0;
}

void loop() {
  int16_t i;
  if (mode == 0) {
    // Disp: InIT
    num_disp[0] = CHAR_I;
    num_disp[1] = CHAR_N;
    num_disp[2] = CHAR_I;
    num_disp[3] = CHAR_T;
    tm1637.display(num_disp);

    // Initialize the Controller
    delay(500);
    PressB();
    delay(500);
    PressB();

    // Disp: CLEA
    num_disp[0] = 0x0C;
    num_disp[1] = CHAR_L;
    num_disp[2] = 0x0E;
    num_disp[3] = 0x0A;
    tm1637.display(num_disp);
    delay(L_INTV);
    
    mode = 1;
    refresh = 1;
  } else if (mode > 0 && mode < 20) {
    /** mode specification:
     * 1. Input year
     * 2. Input month
     * 3. Input day
     * 4. Entry of R+3/4/5
     */
    if (refresh) {
      refresh = 0;
      switch (mode) {
      case 1:
        DispNDigit(3, 4, year);
        break;

      case 2:
        DispNDigit(3, 2, month);
        break;
      
      case 3:
        DispNDigit(3, 2, day);
        break;
      }
    }
    if (btn_stat & 1) {
      btn_stat &= (~1);
      switch (mode) {
      case 1:
        ++year;
        refresh = 1;
        break;
      
      case 2:
        ++month;
        if (month > 12) month = 1;
        refresh = 1;
        break;
      
      case 3:
        ++day;
        if (day > month_day[month] + (month == 2 ? is_leap_y : 0)) day = 1;
        refresh = 1;
        break;

      default:
        break;
      }
    } else if (btn_stat & 2) {
      btn_stat &= (~2);
      switch (mode) {
      case 1:
        --year;
        refresh = 1;
        break;
      
      case 2:
        --month;
        if (month < 1) month = 12;
        refresh = 1;
        break;
      
      case 3:
        --day;
        if (day < 1) day = month_day[month] + (month == 2 ? is_leap_y : 0);
        refresh = 1;
        break;

      default:
        break;
      }
    } else if (btn_stat & 4) {
      btn_stat &= (~4);
      switch (mode) {
      case 2:
        mode = 1;
        refresh = 1;
        break;
      
      case 3:
        day = 1;
        mode = 2;
        refresh = 1;
        break;

      default:  // 1
        break;
      }
    } else if (btn_stat & 8) {
      btn_stat &= (~8);
      switch (mode) {
      case 1:
        is_leap_y = IsLeapYear();
        mode = 2;
        refresh = 1;
        break;

      case 2:
        mode = 3;
        refresh = 1;
        break;
      
      case 3:
        mode = 4;
        refresh = 1;
        break;

      default:
        break;
      }
    }
  } else if (mode >= 20) {
    // TODO
  }
}

void GameToDate() {
  int8_t i;
  MoveCursor(Hat::BOTTOM);
  for (i = 0; i < 4; ++i) {
    MoveCursor(Hat::RIGHT);
  }
  // Setting
  PressA();
  delay(L_INTV);
  // Down to bottom
  for (i = 0; i < 14; ++i) {
    MoveCursor(Hat::BOTTOM);
  }
  // Console
  PressA();
  delay(M_INTV);
  for (i = 0; i < 4; ++i) {
    MoveCursor(Hat::BOTTOM);
  }
  // DateAndTime
  PressA();
  delay(M_INTV);
  for (i = 0; i < 2; ++i) {
    MoveCursor(Hat::BOTTOM);
  }
}

void DateToGame() {
  int8_t i;
  // Console -> Setting -> Main Page
  for (i = 0; i < 3; ++i) {
    PressB();
    delay(M_INTV);
  }
  MoveCursor(Hat::TOP);
  for (i = 0; i < 2; ++i) {
    MoveCursor(Hat::LEFT);
  }
}

void DatePlusOne() {
  // Up for 1 day
  MoveCursor(Hat::TOP);
  ++day;

  if (day > month_day[month] + (month == 2 ? is_leap_y : 0)) {
    day = 1;
    // Month
    MoveCursor(Hat::LEFT);
    // Up for 1 month
    MoveCursor(Hat::TOP);
    ++month;

    if (month == 13) {
      month = 1;
      // Year
      MoveCursor(Hat::LEFT);
      // Up for 1 year
      MoveCursor(Hat::TOP);
      ++year;
      // reCalc leap year flag
      is_leap_y = IsLeapYear();
      // Month
      MoveCursor(Hat::RIGHT);
    }

    // Day
    MoveCursor(Hat::RIGHT);
  }
}

uint8_t IsLeapYear() {
  if ((year % 400 == 0) || ((year % 4 == 0) && (year % 100 != 0)))  return 1;
  else  return 0;
}

void DatePlusN(int32_t days) {
  int8_t i;
  
  if (days > 0) {
    --days;
    
    // Year
    PressA();
    delay(M_INTV);
    // Day
    for (i = 0; i < 2; ++i) {
      MoveCursor(Hat::RIGHT);
    }
    DatePlusOne();
    // OK
    for (i = 0; i < 3; ++i) {
      MoveCursor(Hat::RIGHT);
    }
    // Confirm Date
    PressA();
    delay(M_INTV);
  }
  while (days > 0) {
    --days;
    
    PressA();
    delay(M_INTV);
    for (i = 0; i < 3; ++i) {
      MoveCursor(Hat::LEFT);
    }
    DatePlusOne();
    for (i = 0; i < 3; ++i) {
      MoveCursor(Hat::RIGHT);
    }
    PressA();
    delay(M_INTV);
  }
}

void DispNDigit(int8_t end, int8_t n, int32_t num) {
  if (end > 3) end = 3;
  if (end < 0) end = 0;
  if (n > end + 1) n = end + 1;
  if (n < 0) n = 0;
  
  int8_t i;
  for (i = 0; i < 4; ++i)
    num_disp[i] = BLANK;
  for (i = end; n > 0; --i, --n, num /= 10) {
    num_disp[i] = num % 10;
  }
  tm1637.display(num_disp);
}

void MoveCursor(Hat hat) {
  SwitchControlLibrary().MoveHat((uint8_t)hat);
  delay(S_INTV);
  SwitchControlLibrary().MoveHat((uint8_t)Hat::CENTER);
  delay(S_INTV);
}

void PressA() {
  SwitchControlLibrary().PressButtonA();
  delay(S_INTV);
  SwitchControlLibrary().ReleaseButtonA();
  delay(S_INTV);
}

void PressB() {
  SwitchControlLibrary().PressButtonB();
  delay(S_INTV);
  SwitchControlLibrary().ReleaseButtonB();
  delay(S_INTV);
}

void PressX() {
  SwitchControlLibrary().PressButtonX();
  delay(S_INTV);
  SwitchControlLibrary().ReleaseButtonX();
  delay(S_INTV);
}

void PressHome() {
  SwitchControlLibrary().PressButtonHome();
  delay(S_INTV);
  SwitchControlLibrary().ReleaseButtonHome();
  delay(S_INTV);
}

void Btn1Pressed() {
  if (millis() - last_debounce_time[0] > DEBOUNCE_DELAY && (last_btn_stat & 1)) {
    btn_stat |= 1;
  }
  last_debounce_time[0] = millis();
  last_btn_stat ^= 1;
}

void Btn2Pressed() {
  if (millis() - last_debounce_time[1] > DEBOUNCE_DELAY && (last_btn_stat & 2)) {
    btn_stat |= 2;
  }
  last_debounce_time[1] = millis();
  last_btn_stat ^= 2;
}

void Btn3Pressed() {
  if (millis() - last_debounce_time[2] > DEBOUNCE_DELAY && (last_btn_stat & 4)) {
    btn_stat |= 4;
  }
  last_debounce_time[2] = millis();
  last_btn_stat ^= 4;
}

void Btn4Pressed() {
  if (millis() - last_debounce_time[3] > DEBOUNCE_DELAY && (last_btn_stat & 8)) {
    btn_stat |= 8;
  }
  last_debounce_time[3] = millis();
  last_btn_stat ^= 8;
}
