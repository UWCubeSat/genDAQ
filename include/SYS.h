
#pragma once
#include <Arduino.h>
#include <GlobalTools.h>

class System_;
struct CLK_CONFIG;

///////////////////////////////////////////////////////////////////////////////////////////////////
///// SECTION -> SYSTEM CONTROL
///////////////////////////////////////////////////////////////////////////////////////////////////

// System Manager
class System_ {

  public:


    // Clock & Oscillator Utility
    struct CLKUtil {

      bool initialize();

      bool enableClock();

      bool disableClock();

      private:
        friend System_;
        const System_ *super;
        explicit CLKUtil(System_ *sys);

        bool sysInit;
        bool xoscInit;

    }clk{this}; 

    // Memory Utility
    struct MEMUtil {

      private:
        friend System_;
        const System_ *super;
        explicit MEMUtil(System_ *sys) : super(sys){}
    }mem{this};


    // Event Utility
    struct EVNTUtil {

      private:
        friend System_;
        const System_ *super;
        explicit EVNTUtil(System_ *sys) : super(sys){}
    }evnt{this};


    // Error Utility
    struct ERRUtil {

      private:
        friend System_;
        const System_ *super;
        explicit ERRUtil(System_ *sys) : super(sys){}
    }err{this};


    // Pin/Port Utility
    struct IOUtil {

      private:
        friend System_;
        System_ *super;
        explicit IOUtil(System_ *sys) : super(sys){}
    }io{this};


    // Watch Dog Timer Utility
    struct WDTUtil {

      private:
        friend System_;
        System_ *super;
        explicit WDTUtil(System_ *sys) : super(sys){}
    }wdt{this};

    
    // System Core Utilities 
    struct COREUtil {

      private:
        friend System_;
        System_ *super;
        explicit COREUtil(System_ *sys) : super(sys){}
    }core{this};


  private:
    System_() {}



};
extern System_ &System;

