#include <limits>
#include <stdio.h>

#include "hadrware.h"
#include "setup_data.h"
#include "ILI9327_Shield.h"

struct Vec5 {
    int32_t data[5];
    
    Vec5() = default;
    constexpr Vec5(int resolution, int a1, int a2, int a3, int a4, int a5)
    {
        data[0] = a1 * resolution / 100;
        data[1] = a2 * resolution / 100;
        data[2] = a3 * resolution / 100;
        data[3] = a4 * resolution / 100;
        data[4] = a5 * resolution / 100;
    }
    
    int32_t operator[](int i) const {return data[i];}
    
    int64_t operator*(const Vec5& other) const
    {
        int64_t result = 0;
        for(int i = 0; i < 5; i++) result += int64_t(data[i]) * other.data[i];
        return result;
    }
    
    int64_t operator*(int64_t value)
    {
        int64_t result = 0;
        for(int i = 0; i < 5; i++) result += int64_t(data[i]) * value;
        return result;
    }
};

constexpr int32_t touch_scale = std::numeric_limits<int32_t>::max();

void TouchSetup::calibrate()
{
  Vec5 raw_x, raw_y;
  Vec5 xd (RES_X, 50, 20, 20, 80, 80);
  Vec5 yd (RES_Y, 50, 80, 20, 20, 80);

  lcd.DRect(0, 0, RES_X, RES_Y, 0);
  lcd.text2("Touchscreen calibration", -1, 0);
  lcd.text("Press all crosses in order", -1, 40);
  
  TouchConfig().wait_release();
  
  for(int i = 0; i < 5; i++)
  {
    if(i > 0) lcd.WRect(xd[i - 1] - 1, yd[i - 1] - 1, 3, 3, 0);
    lcd.WRect(xd[i] - 1, yd[i] - 1, 3, 3, -1);
    
    TouchConfig touch;
    touch.wait_press();
    raw_x.data[i] = touch.x;
    raw_y.data[i] = touch.y;
    touch.wait_release();
  }
  lcd.DRect(0, 0, RES_X, RES_Y, 0);
  
  int64_t a = raw_x * raw_x;
  int64_t b = raw_y * raw_y;
  int64_t c = raw_x * raw_y;
  int64_t d = raw_x * 1;
  int64_t e = raw_y * 1;
  int64_t X1 = raw_x * xd;
  int64_t X2 = raw_y * xd;
  int64_t X3 = xd * 1;
  int64_t Y1 = raw_x * yd;
  int64_t Y2 = raw_y * yd;
  int64_t Y3 = yd * 1;
  
  int64_t dt = 5 * (a * b - c * c) + 2 * c * d * e - a * e * e - b * d * d;

#define LINE(dst, src) \
  int64_t dst##1 = 5 * (src##1 * b - src##2 * c) + e * (src##2 * d - src##1 * e) + src##3 * (c * e -  b * d); \
  int64_t dst##2 = 5 * (src##2 * a - src##1 * c) + d * (src##1 * e - src##2 * d) + src##3 * (c * d -  a * e); \
  int64_t dst##3 = src##3 * (a * b - c * c) + src##1 * (c * e - b * d) + src##2 * (c * d -  a * e)

  LINE(dx, X);
  LINE(dy, Y);
#undef LINE
  
  A = int32_t((double)dx1 / (double)dt * touch_scale + 0.5);
  B = int32_t((double)dx2 / (double)dt * touch_scale + 0.5);
  C = int32_t((double)dx3 / (double)dt * touch_scale + 0.5);
  D = int32_t((double)dy1 / (double)dt * touch_scale + 0.5);
  E = int32_t((double)dy2 / (double)dt * touch_scale + 0.5);
  F = int32_t((double)dy3 / (double)dt * touch_scale + 0.5);
  
  sync();
}

int32_t TouchSetup::x(int32_t x, int32_t y)
{
  return int32_t(int64_t(A) * x + int64_t(B) * y + C);
}

int32_t TouchSetup::y(int32_t x, int32_t y)
{
  return int32_t(int64_t(D) * x + int64_t(E) * y + F);
}
