#pragma once

#include <vector>

#include "odb/db.h"

namespace tool {

enum FlopPinInput {
    D,
    D0,
    D1,
    D2,
    D3,
    SI,
    CLK
};

enum FlopPinOutput {
    Q,
    Q0,
    Q1,
    Q2,
    Q3,
    SO
};

class FlipFlop
{
public:
  FlipFlop(std::string name,
           int id,
           int x,
           int y,
           int size,
           float delay,
           float power,
           int area)
    : name_(name),
      id_(id),
      x_(x),
      y_(y),
      size_(size),
      delay_(delay),
      power_(power),
      area_(area)
  {}

  ~FlipFlop() = default;

  std::string getName();
  void setLocation(int x, int y);
  void getLocation(int& x, int& y);
  void setDelay(float delay);
  float getDelay();
  void setPower(float power);
  float getPower();

private:
  std::string name_;
  int id_;
  int x_;
  int y_;
  int size_;
  float delay_;
  float power_;
  int area_;
  std::vector<int> inputs;
  std::vector<int> outputs;


};
}