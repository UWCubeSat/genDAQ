///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Pre-Processor (Non-Defs)
///////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Board Properties ==>> Prefix: BRD__
///////////////////////////////////////////////////////////////////////////////////////////////////

#define BRD__TOTAL_RAM 192000

///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Global Settings ==>> Prefix: PROG__
///////////////////////////////////////////////////////////////////////////////////////////////////

#define PROG__DEBUG_MODE 1 // 1 = Debug mode -> print things, 0 = Standard mode -> dont print. 


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Communication Protocol ==>> Prefix: PRTC__
///////////////////////////////////////////////////////////////////////////////////////////////////

#define PRTC__NONE 0


///////////////////////////////////////////////////////////////////////////////////////////////////
//// SECTION -> Enums ==>> Suffix: _ID, Preffix: Same & Related to Their System/Class. 
///////////////////////////////////////////////////////////////////////////////////////////////////

// NOTE =>> ALL ENUMS MUST END WITH _ID

const enum SETTING_ID : uint8_t {
  SETTING_NONE = PRTC__NONE
};
 
const enum ERROR_ID : uint8_t {
  ERROR_NONE = PRTC__NONE
};

const enum RESTART_FLAG_ID : uint8_t {
  RESTART_NONE = PRTC__NONE
};



