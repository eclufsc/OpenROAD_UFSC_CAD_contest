#include "tool/FlipFlop.h"

namespace tool {

  std::string FlipFlop::getName()
  {
    return name_;
  } 
  
  void FlipFlop::setLocation(int x, int y)
  {
    x_ = x;
    y_ = y;
  }

  void FlipFlop::getLocation(int& x, int& y)
  {
    x = x_;
    y = y_;
  }

  void FlipFlop::setDelay(float delay)
  {
    delay_ = delay;
  }

  float FlipFlop::getDelay()
  {
    return delay_;
  }

  void FlipFlop::setPower(float power)
  {
    power_ = power;
  }

  float FlipFlop::getPower()
  {
    return power_;
  }
}