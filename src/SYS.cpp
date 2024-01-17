
#include <SYS.h>


///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> CLOCK & OSCILLATOR UTILITY (CLK)
///////////////////////////////////////////////////////////////////////////////////////////////////

bool System_::CLKUtil::initialize() {

  // Iniitalize 32k oscillators
  OSC32KCTRL->XOSC32K.reg = OSC32KCTRL_XOSC32K_RESETVALUE;
  OSC32KCTRL->XOSC32K.reg |= 
      OSC32KCTRL_XOSC32K_ENABLE   // Enable external crystal oscillator 
    | OSC32KCTRL_XOSC32K_EN1K     // Enable 32khz output
    | OSC32KCTRL_XOSC32K_EN32K    // Enable 1khz output
    | OSC32KCTRL_XOSC32K_XTALEN;  // Enable signal output

  OSC32KCTRL->OSCULP32K.reg = OSC32KCTRL_OSCULP32K_RESETVALUE;
  OSC32KCTRL->OSCULP32K.reg |= 
      OSC32KCTRL_OSCULP32K_EN32K   // Enable 32khz output
    | OSC32KCTRL_OSCULP32K_EN1K;   // Enable 1khz output

  OSC32KCTRL->CFDCTRL.reg = OSC32KCTRL_CFDCTRL_RESETVALUE;
  OSC32KCTRL->CFDCTRL.reg |= 
      OSC32KCTRL_CFDCTRL_SWBACK     // Enable clock switching on failiure
    | OSC32KCTRL_CFDCTRL_CFDEN;     // Enable clock failiure detection

  xoscInit = true;
  while(!OSC32KCTRL->STATUS.bit.XOSC32KRDY) {
    if (OSC32KCTRL->STATUS.bit.XOSC32KFAIL) {
      xoscInit = false;
    }
  }

  // Enable regular oscillators




  // Setup generic clocks

  GCLK->CTRLA.bit.SWRST = 1;
  while(GCLK->SYNCBUSY.reg & GCLK_SYNCBUSY_SWRST);
  

}