#include <SwitchControlLibrary.h>
#include "TM1637.h"

#define S_INTV 40
#define M_INTV 160
#define L_INTV 500
#define XL_INTV 950

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
uint16_t year, init_year;
uint8_t month, day, init_month, init_day, is_leap_y;

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
void PressR();
void PressHome();

void DatePlusOne();
uint8_t IsLeapYear();

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
     * 4. Entry of +3
     * 5. Entry of R+1 S+3
     * 6. Entry of R+3/4/5
     * 7. Entry of Capture and Levelup
     * 8. Entry of +N with watt
     * 9. Entry of +N without watt
     * 10. R+3/4/5 - Input days
     * 11. +3 - Confirm
     * 12. R+1 S+3 - Confirm
     * 13. Capture and Levelup - Confirm
     * 14. +N with watt - Input days
     * 15. +N without watt - Input days
     */

    if (refresh) {  // refresh display
      refresh = 0;
      switch (mode) {
      case 1: // yy:yy
        DispNDigit(3, 4, year);
        break;

      case 2: // M_:mm
        DispNDigit(3, 2, month);
        num_disp[0] = 0x0E;
        break;
      
      case 3: // d_:dd
        DispNDigit(3, 2, day);
        num_disp[0] = 0x0d;
        break;
      
      case 4: // 1_:_3
        num_disp[0] = 1;
        num_disp[1] = num_disp[2] = BLANK;
        num_disp[3] = 3;
        break;
      
      case 5: // 2_:1S
        DispNDigit(3, 2, 15);
        num_disp[0] = 2;
        break;

      case 6: // 3_:R3
        num_disp[0] = num_disp[3] = 3;
        num_disp[1] = BLANK;
        num_disp[2] = 0x0A;
        break;
      
      case 7: // 4_:CA
        num_disp[0] = 4;
        num_disp[1] = BLANK;
        num_disp[2] = 0x0C;
        num_disp[3] = 0x0A;
        break;
      
      case 8: // 5_:ny
        num_disp[0] = 5;
        num_disp[1] = BLANK;
        num_disp[2] = CHAR_N;
        num_disp[3] = CHAR_Y;
        break;
      
      case 9: // 6_:nn
        num_disp[0] = 6;
        num_disp[1] = BLANK;
        num_disp[2] = num_disp[3] = CHAR_N;
        break;
      
      case 10:// R_:0d
        DispNDigit(3, 2, day_cnt);
        num_disp[0] = 0x0A;
        break;
      
      case 11:// _y:n_
      case 12:
      case 13:
        num_disp[1] = num_disp[2] = BLANK;
        num_disp[0] = CHAR_Y;
        num_disp[3] = CHAR_N;
        break;
      
      case 14:// dd:dd
      case 15:
        // maintain
        break;

      default:
        DispNDigit(3, 0, 1);  // Clear Display
        break;
      }
      tm1637.point(1);
      tm1637.display(num_disp);
    }

    if (btn_stat & 1) { // key 1 as up
      btn_stat &= (~1);
      refresh = 1;
      switch (mode) {
      case 1:
        ++year;
        break;
      
      case 2:
        ++month;
        if (month > 12) month = 1;
        break;
      
      case 3:
        ++day;
        if (day > month_day[month] + (month == 2 ? is_leap_y : 0)) day = 1;
        break;
      
      case 4:
        mode = 9;
        break;
      
      case 5:
      case 6:
      case 7:
      case 8:
      case 9:
        --mode;
        break;

      case 10:
        ++day_cnt;
        if (day_cnt > 5) day_cnt = 3;
        break;
      
      case 14:
      case 15:
        ++num_disp[cur_pos];
        if (num_disp[cur_pos] > 9) num_disp[cur_pos] = 0;
        if (!(num_disp[0] | num_disp[1] | num_disp[2] | num_disp[3])) num_disp[3] = 1;
        break;
      
      default:  // 11, 12, 13
        break;
      }
    } else if (btn_stat & 2) {  // key 2 as down (or right)
      btn_stat &= (~2);
      refresh = 1;
      switch (mode) {
      case 1:
        --year;
        break;
      
      case 2:
        --month;
        if (month < 1) month = 12;
        break;
      
      case 3:
        --day;
        if (day < 1) day = month_day[month] + (month == 2 ? is_leap_y : 0);
        break;
      
      case 4:
      case 5:
      case 6:
      case 7:
      case 8:
        ++mode;
        break;
      
      case 9:
        mode = 4;
        break;

      case 10:
        --day_cnt;
        if (day_cnt < 3) day_cnt = 5;
        break;
      
      case 14:
      case 15:
        ++cur_pos;
        if (cur_pos > 3) cur_pos = 0;
        break;

      default:  // 11, 12, 13
        break;
      }
    } else if (btn_stat & 4) {  // key 3 as B (cancel)
      btn_stat &= (~4);
      refresh = 1;
      switch (mode) {
      case 2:
        mode = 1;
        break;
      
      case 3:
        day = 1;
        mode = 2;
        break;
      
      case 10:
        mode = 6;
        break;
      
      case 11:
        mode = 4;
        break;
      
      case 12:
        mode = 5;
        break;
      
      case 13:
        mode = 7;
        break;
      
      case 14:
        mode = 8;
        break;
      
      case 15:
        mode = 9;
        break;

      default:  // 1, 4, 5, 6, 7, 8, 9
        break;
      }
    } else if (btn_stat & 8) {  // key 4 as A (confirm)
      btn_stat &= (~8);
      refresh = 1;
      switch (mode) {
      case 1:
        init_year = year;
        is_leap_y = IsLeapYear();
        mode = 2;
        break;

      case 2:
        init_month = month;
        mode = 3;
        break;
      
      case 3:
        init_day = day;
        mode = 4;
        break;
      
      case 4:
        day_cnt = 3;
        mode = 11;
        break;
      
      case 5:
        day_cnt = 1;
        mode = 12;
        break;

      case 6:
        day_cnt = 3;
        mode = 10;
        break;
      
      case 7:
        mode = 13;
        break;
      
      case 8:
        DispNDigit(3, 4, 1);
        cur_pos = 0;
        mode = 14;
        break;
      
      case 9:
        DispNDigit(3, 4, 1);
        cur_pos = 0;
        mode = 15;
        break;
      
      case 10:
        mode = 20;
        break;
      
      case 11:
        mode = 22;
        break;
      
      case 12:
        mode = 30;
        break;
      
      case 13:
        mode = 70;
        break;
      
      case 14:
        day_cnt = 0;
        for (i = 0; i < 4; ++i) {
          day_cnt = day_cnt * 10 + num_disp[i];
        }
        mode = 50;
        break;
      
      case 15:
        day_cnt = 0;
        for (i = 0; i < 4; ++i) {
          day_cnt = day_cnt * 10 + num_disp[i];
        }
        mode = 60;
        break;

      default:
        break;
      }
    }
  } else if (mode >= 20) {
    /** mode specification:
     * 
     * 20 - 29: R+3/4/5
     * 20. 关游戏
     * 21. 开游戏
     * 22. 领瓦特
     * 23. 开始招募
     * 24. Game to Date
     * 25. Date Plus One
     * 26. Date to Game
     * 27. 退出招募
     * 28. 领瓦特（终止）
     * 
     * 30 - 49: R+1 S+3
     * 30-37. Same as 20-27
     * 38. Game to Date
     * 39. Back to Now
     * 40. Date to Game
     * 41. Save
     * 
     * 50 - 59: +N with watt        // TODO
     * 50. 领瓦特，并返回
     * 51. Game to Date
     * 52. Date Plus One
     * 53. Date to Game
     * 
     * 60 - 69: +N without watt
     * 60. Game to Date
     * 61. Date Plus One
     * 62. Date Plus N
     * 63. Back to Now
     * 64. Date to Game
     * 65. Save
     * 
     * 70 - 79: Capture and Levelup // TODO
     * 70. 捕捉，返回游戏界面
     * 71. 打开背包，喂糖
     * 72. 取消学习技能（取消进化）
     * 73. 退出背包，查看宝可梦信息
     * 
     */
    if (refresh) {
      refresh = 0;
      if (mode >= 20 && mode < 50) {
        DispNDigit(3, 2, day_cnt);
        num_disp[0] = (mode < 30) ? 0x0A : 5;
      } else if (mode >= 50 && mode < 70) {
        DispNDigit(3, 4, day_cnt);
      }
      tm1637.point(0);
      tm1637.display(num_disp);
    }

    // Switch Controller Operations
    switch (mode) {
    case 20:  // Shutdown game
    case 30:
      PressHome();
      delay(L_INTV);
      PressX();
      delay(M_INTV);
      PressA();
      delay(2500);  // game terminating...
      break;
    
    case 21:  // Start game
    case 31:
      PressA();
      delay(XL_INTV);
      PressA();
      delay(1000 * 18); // game starting...
      for (i = 0; i < 10; ++i)
        PressA();
      delay(1000 * 10); // loading...
      break;
    
    case 22:  // Get watt
    case 28:
    case 32:
      PressA();
      delay(L_INTV);
      PressA();
      delay(L_INTV);
      PressA();
      delay(L_INTV);
      break;
    
    case 23:
    case 33:
      PressA();
      delay(2500);  // recruitment starting...
      break;
    
    case 24:  // From game to date setting
    case 34:
    case 38:
    case 60:
      PressHome();
      delay(L_INTV);
      MoveCursor(Hat::BOTTOM);
      for (i = 0; i < 4; ++i) {
        MoveCursor(Hat::RIGHT);
      }
      // Setting
      PressA();
      delay(XL_INTV);
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
      break;
    
    case 25:  // the first day
    case 35:
    case 61:
      MoveCursor(Hat::BOTTOM);
      MoveCursor(Hat::BOTTOM);
      // Year
      PressA();
      delay(M_INTV);
      // Month
      MoveCursor(Hat::RIGHT);
      // Day
      MoveCursor(Hat::RIGHT);
      DatePlusOne();
      // OK
      for (i = 0; i < 3; ++i) {
        MoveCursor(Hat::RIGHT);
      }
      PressA();
      delay(M_INTV);
      break;
    
    case 26:  // Back to game from date setting
    case 36:
    case 40:
    case 64:
      // Console -> Setting -> Main Page
      for (i = 0; i < 3; ++i) {
        PressB();
        delay(M_INTV);
      }
      MoveCursor(Hat::TOP);
      MoveCursor(Hat::LEFT);
      MoveCursor(Hat::LEFT);
      PressA();
      delay(L_INTV);
      break;
    
    case 27:
    case 37:
      PressB();
      delay(L_INTV);
      PressA();
      delay(4500);  // recruitment stopping...
      break;
    
    case 63:
      MoveCursor(Hat::TOP);
      MoveCursor(Hat::TOP);
    case 39:  // Reset date
      year = init_year;
      month = init_month;
      day = init_day;
      is_leap_y = IsLeapYear();
      PressA();
      PressA();
      break;
    
    case 41:  // Save
    case 65:
      PressX();
      delay(L_INTV);
      PressR();
      delay(1500);
      PressA();
      delay(3000);
      break;
    
    case 62:  // Plus one day (not 1st day)
      PressA();
      delay(80);
      // to day
      for (i = 0; i < 3; ++i) {
        MoveCursor(Hat::LEFT);
      }
      DatePlusOne();
      // OK
      for (i = 0; i < 3; ++i) {
        MoveCursor(Hat::RIGHT);
      }
      PressA();
      delay(80);
      break;

    }

    // Mode Changing
    switch (mode) {

    // 20 - 26
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    // 30 - 40
    case 30:
    case 31:
    case 32:
    case 33:
    case 34:
    case 35:
    case 36:
    case 37:
    case 38:
    case 39:
    case 40:
    // 60, 63
    case 60:
    case 63:
      ++mode;
      break;
    
    case 27:
      --day_cnt;
      refresh = 1;
      if (day_cnt > 0)  mode = 22;
      else mode = 28;
      break;
    
    case 28:
      mode = 6;
      refresh = 1;
      break;
    
    case 41:
      day_cnt = 3;
      refresh = 1;
      mode = 22;
      break;
    
    case 61:
    case 62:
      --day_cnt;
      if (day_cnt % 200 == 0)
        mode = 63;
      else
        mode = 62;
      refresh = 1;
      break;
    
    case 64:
      if (day_cnt > 0)
        ++mode;
      else {
        mode = 9;
        refresh = 1;
      }
      break;

    case 65:
      mode = 60;
      break;

    default:
      mode = 6;
      refresh = 1;
      break;
    }

    // Press B then stop
    if (btn_stat & 4) {
      btn_stat &= (~4);
      if (mode >= 20 && mode < 30) mode = 6;
      else if (mode >= 30 && mode < 50) mode = 5;
      else if (mode >= 60 && mode < 70) mode = 9;
      refresh = 1;
    }
    btn_stat &= (~11);
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

void PressR() {
  SwitchControlLibrary().PressButtonR();
  delay(S_INTV);
  SwitchControlLibrary().ReleaseButtonR();
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
