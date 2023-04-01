/*
__ __| |           |  /_) |     ___|             |           |
  |   __ \   _ \  ' /  | |  / |      _ \ __ \   |      _` | __ \   __|
  |   | | |  __/  . \  |   <  |   |  __/ |   |  |     (   | |   |\__ \
 _|  _| |_|\___| _|\_\_|_|\_\\____|\___|_|  _| _____|\__,_|_.__/ ____/
-----------------------------------------------------------------------------
TKGL_MIDIMAPPER  custom mapping library .
This is a custom midi mapping library for TKGL_MIDIMAPPER LD_PRELOAD library

----------------------------------------------------------------------------
LAUNCHPAD MK2 FOR IAMFORCE
----------------------------------------------------------------------------

  Disclaimer.
  This work is licensed under the Creative Commons Attribution-NonCommercial 4.0 International License.
  To view a copy of this license, visit http://creativecommons.org/licenses/by-nc/4.0/
  or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
  NON COMMERCIAL - PERSONAL USE ONLY : You may not use the material for pure
  commercial closed code solution without the licensor permission.
  You are free to copy and redistribute the material in any medium or format,
  adapt, transform, and build upon the material.
  You must give appropriate credit, a link to the github site
  https://github.com/TheKikGen/USBMidiKliK4x4 , provide a link to the license,
  and indicate if changes were made. You may do so in any reasonable manner,
  but not in any way that suggests the licensor endorses you or your use.
  You may not apply legal terms or technological measures that legally restrict
  others from doing anything the license permits.
  You do not have to comply with the license for elements of the material
  in the public domain or where your use is permitted by an applicable exception
  or limitation.
  No warranties are given. The license may not give you all of the permissions
  necessary for your intended use.  This program is distributed in the hope that
  it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

/*
-------------------------------------------------------------------------------
MAPPING CONFIGURATION WITH FORCE BUTTONS AND FUNCTIONS - LAUNCHPAD MINI MK3


- UP BUTTON aka "Controller SHIFT"
The UP button is used as a "controller SHIFT" button.  This is not the same
"shift" as the one present on the MPC, i.e. it is Laucnhchpad MINI specific.

- DOWN BUTTON :
. Go down in the clip matrix.
. if controller shift holded : go UP in the matrix.


- LEFT BUTTON :
. equivalent to Force copy clip button
. if controller shift holded : go left in the pad matrix

- RIGHT BUTTON :
. equivalent to Force delete clip button
. if controller shift holded : go right in the pad matrix


- "STOP SOLO MUTE" button aka SSM

If "Controller SHIFT" is holded, the SSM button will send a Force LAUNCH 8.

If SSM is holded, the 2 "columns pads" lines are showed on line 7 and 8.  It is
then possible to select the track or mute/solo/stop clip/rec arm tracks by
pressing the right pad.  First line is corresponding to track select, and second
is solo modes.

SSM holded :
+ Pads = Select track and swith solo modes on/Off

+ Pad on the first line = will show track edit on the selected pad column

+ Launch 7 = change current solo mode . The SSM button color will change
accordingly to the current mode : red = rec arm, orange = mute, blue = solo,
green = clip stop.

+ LAUNCH 6 = STOP ALL CLIPS. Same behavious as Force

+ Launch 5 = MPC SHIFT key.  This can be used to show options settings on the 2
last lines rather than solo modes (same behaviour as Force).  You can use that
to set metronome on/off or transpose the keyboard for example.

+ Launch 4 = lock the SSM mode ON/OFF. The 2 last lines of the Launchpad will
show columns pads permanenently.

- LAUNCH 1-7
. If not in controller shift mode or SSM mode, equivalent to Force launch buttons.

- LAUNCH 8 (SSM button)
. if controller shift mode is holded, that will generate a Force launch 8 button.

- SESSION button :
. if controller shift mode is holded : MASTER
. else : LAUNCH

- USER button : STEP SEQ

- KEYS button :   NOTE

-------------------------------------------------------------------------------
*/

#define IAMFORCE_DRIVER_VERSION "1.0"
#define IAMFORCE_DRIVER_ID "LPMK2"
#define IAMFORCE_DRIVER_NAME "Novation Launchpad Mini Mk2"
#define IAMFORCE_ALSASEQ_DEFAULT_CLIENT_NAME "Launchpad Mini MK2"
#define IAMFORCE_ALSASEQ_DEFAULT_PORT 1

//#define LP_DEVICE_ID 0x0D // Launchpad Mini MK3
//#define LP_DEVICE_ID 0x0C // Launchpad X

#define LP_DEVICE_ID 0x18 // Launchpad MK2

#define LP_SYSEX_HEADER 0xF0,0x00,0x20,0x29,0x02, LP_DEVICE_ID

#define CTRL_BT_UP 0x68
#define CTRL_BT_DOWN 0x69
#define CTRL_BT_LEFT 0x6A
#define CTRL_BT_RIGHT 0x6B
#define CTRL_BT_SESSION 0x6C
#define CTRL_BT_USER1 0x6D
#define CTRL_BT_USER2 0x6E
#define CTRL_BT_MIXER 0x62
#define CTRL_BT_LAUNCH_1 0x59  // Also Volume
#define CTRL_BT_LAUNCH_2 0x4F  // Pan
#define CTRL_BT_LAUNCH_3 0x45  // Send A
#define CTRL_BT_LAUNCH_4 0x3B  // Send B
#define CTRL_BT_LAUNCH_5 0x31  // Stop
#define CTRL_BT_LAUNCH_6 0x27  // Mute
#define CTRL_BT_LAUNCH_7 0x1D  // Solo
#define CTRL_BT_LAUNCH_8 0x13  // Record Arm


// Pads start at 0 from bottom left to upper right
#define CTRL_PAD_NOTE_OFFSET 0
#define CTRL_PAD_MAX_LINE 8
#define CTRL_PAD_MAX_COL  8


// Standard colors
#define CTRL_COLOR_BLACK 0
#define CTRL_COLOR_DARK_GREY 1
#define CTRL_COLOR_GREY 2
#define CTRL_COLOR_WHITE 3
#define CTRL_COLOR_RED 5
#define CTRL_COLOR_RED_LT 0x07
#define CTRL_COLOR_AMBER 9
#define CTRL_COLOR_YELLOW 13
#define CTRL_COLOR_LIME 17
#define CTRL_COLOR_GREEN 21
#define CTRL_COLOR_SPRING 25
#define CTRL_COLOR_TURQUOISE 29
#define CTRL_COLOR_CYAN 33
#define CTRL_COLOR_SKY 37
#define CTRL_COLOR_OCEAN 41
#define CTRL_COLOR_BLUE 45
#define CTRL_COLOR_ORCHID 49
#define CTRL_COLOR_MAGENTA 53
#define CTRL_COLOR_PINK 57



// SYSEX for Launchpad mini Mk3

const uint8_t SX_DEVICE_INQUIRY[] = { 0xF0, 0x7E, 0x7F, 0x06, 0x01, 0xF7 };
const uint8_t SX_LPMK2_INQUIRY_REPLY_APP[] = { 0xF0, 0x7E, 0x00, 0x06, 0x02, 0x00, 0x20, 0x29, 0x13, 0x01, 0x00, 0x00 } ;

//const uint8_t SX_LPMK2_USER_MODE[]  = { LP_SYSEX_HEADER, 0x0E, 0x01, 0xF7 } ;
//F0h 00h 20h 29h 02h 18h 22h <Layout> F7
const uint8_t SX_LPMK2_USER2_MODE[]  = { LP_SYSEX_HEADER, 0x22, 0x02, 0xF7 } ;

// Host >> Launchpad MK2: F0h 00h 20h 29h 02h 18h 14h <Colour> <Loop> <Text> F7h
const uint8_t SX_LPMK2_TEXT_SCROLL[] = { LP_SYSEX_HEADER, 0x14 };

// The brightness of the individual red, green and blue elements of the LED can be controlled to 
// create any colour. Each element has a brightness value from 00h – 3Fh (0 – 63), where 0 is off and 
// 3Fh is full brightness. 
// § Light LED using SysEx (RGB mode) 
// Host >> Launchpad MK2: F0h 00h 20h 29h 02h 18h 0Bh <LED>, <Red> <Green> <Blue> F7h
// Message can be repeated up to 80 times.

uint8_t SX_LPMK2_LED_RGB_COLOR[] = { LP_SYSEX_HEADER, 0x0B, 0x00, 0x00, 0x00, 0x00, 0xF7 };

///////////////////////////////////////////////////////////////////////////////
// LaunchPad Mk2 Specifics
///////////////////////////////////////////////////////////////////////////////
// Scroll a Text on pads
static void ControllerScrollText(const char *message,uint8_t loop,  uint8_t color) {
  uint8_t buffer[128];
  uint8_t *pbuff = buffer;

  memcpy(pbuff,SX_LPMK2_TEXT_SCROLL,sizeof(SX_LPMK2_TEXT_SCROLL));
  pbuff += sizeof(SX_LPMK2_TEXT_SCROLL) ;
  *(pbuff ++ ) = color; //  color
  *(pbuff ++ ) = loop;  // loop

  strcpy(pbuff, message);
  pbuff += strlen(message);
  *(pbuff ++ ) = 0xF7;

  SeqSendRawMidi(TO_CTRL_EXT,  buffer, pbuff - buffer );

}
///////////////////////////////////////////////////////////////////////////////
// LPMMK2 Set a pad RGB Colors
///////////////////////////////////////////////////////////////////////////////
static void ControllerSetPadColorRGB(uint8_t padCt, uint8_t r, uint8_t g, uint8_t b) {

// Host >> Launchpad MK2: F0h 00h 20h 29h 02h 18h 0Bh <LED>, <Red> <Green> <Blue> F7h

  SX_LPMK2_LED_RGB_COLOR[7]  = padCt;
  SX_LPMK2_LED_RGB_COLOR[8]  = r ;
  SX_LPMK2_LED_RGB_COLOR[9]  = g ;
  SX_LPMK2_LED_RGB_COLOR[10] = b ;

  SeqSendRawMidi( TO_CTRL_EXT,SX_LPMK2_LED_RGB_COLOR,sizeof(SX_LPMK2_LED_RGB_COLOR) );
}

///////////////////////////////////////////////////////////////////////////////
// Show a line from Force pad color cache (source = Force pad#, Dest = ctrl pad #)
///////////////////////////////////////////////////////////////////////////////
static void ControllerDrawPadsLineFromForceCache(uint8_t SrcForce, uint8_t DestCtrl) {

    if ( DestCtrl >= CTRL_PAD_MAX_LINE ) return;

    uint8_t pf = SrcForce * 8 ;
    uint8_t pl = ( DestCtrl * 10 ) + 11;

    for ( uint8_t i = 0  ; i <  8 ; i++) {
      ControllerSetPadColorRGB(pl, Force_PadColorsCache[pf].c.r, Force_PadColorsCache[pf].c.g,Force_PadColorsCache[pf].c.b);
      pf++; pl++;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Refresh the pad surface from Force pad cache
///////////////////////////////////////////////////////////////////////////////
static void ControllerRefreshMatrixFromForceCache() {

    for ( int i = 0 ; i< 64 ; i++) {

      // Set pad for external controller eventually
      uint8_t padCt = ( ( 7 - i / 8 ) * 10 + 11 + i % 8 );
      ControllerSetPadColorRGB(padCt, Force_PadColorsCache[i].c.r, Force_PadColorsCache[i].c.g,Force_PadColorsCache[i].c.b);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Controller initialization
///////////////////////////////////////////////////////////////////////////////
static int ControllerInitialize() {

  tklog_info("IamForce : %s implementation, version %s.\n",IAMFORCE_DRIVER_NAME,IAMFORCE_DRIVER_VERSION);

  SeqSendRawMidi( TO_CTRL_EXT,  SX_LPMK2_USER2_MODE, sizeof(SX_LPMK2_USER2_MODE) );
  
  ControllerScrollText("IamForce",0,CTRL_COLOR_OCEAN);
  uint8_t midiMsg[] = {
    0x92, CTRL_BT_UP, CTRL_COLOR_BLUE,
  };

  SeqSendRawMidi(TO_CTRL_EXT,midiMsg,sizeof(midiMsg));
}

///////////////////////////////////////////////////////////////////////////////
// Get a Force pad index from a Launchpad index
///////////////////////////////////////////////////////////////////////////////
static uint8_t ControllerGetForcePadIndex(uint8_t padCt) {
  // Convert pad to Force pad #
  padCt -=  11;
  uint8_t padL  =  7 - padCt  /  10 ;
  uint8_t padC  =  padCt  %  10 ;
  return  padL * 8 + padC ;
}

///////////////////////////////////////////////////////////////////////////////
// Get a Force pad index from a Launchpad index
///////////////////////////////////////////////////////////////////////////////
static uint8_t ControllerGetForcePadNote(uint8_t padCt) {
  return  ControllerGetForcePadIndex(padCt)  + FORCEPADS_NOTE_OFFSET;
}



///////////////////////////////////////////////////////////////////////////////
// Get a Controller pad index from a Force pad index
///////////////////////////////////////////////////////////////////////////////
static int ControllerGetPadIndex(uint8_t padF) {

  if ( padF >= 64 ) return -1;

  return  ( ( 7 - padF / 8 ) * 10 + 11 + padF % 8 ) ;
}

///////////////////////////////////////////////////////////////////////////////
// Map LED lightning messages from Force with Launchpad buttons leds colors
///////////////////////////////////////////////////////////////////////////////
static void ControllerSetMapButtonLed(snd_seq_event_t *ev) {

    int mapVal = -1 ;
    int mapVal2 = -1;

    // TAP LED is better at first ! Always updated....
    // if ( ev->data.control.param == FORCE_BT_TAP_TEMPO )  {
    //       mapVal = CTRL_BT_LOGO;
    //       mapVal2 = ev->data.control.value == 3 ?  CTRL_COLOR_RED_LT:00 ;
    // }
  
    if ( ev->data.control.param == FORCE_BT_NOTE )       mapVal = CTRL_BT_USER2;
    if ( ev->data.control.param == FORCE_BT_STEP_SEQ )   mapVal = CTRL_BT_USER1;

    else if ( ev->data.control.param == FORCE_BT_LAUNCH )     mapVal = CTRL_BT_SESSION ;
    else if ( ev->data.control.param == FORCE_BT_MASTER )     mapVal = CTRL_BT_SESSION ;

    else if ( ev->data.control.param == FORCE_BT_LAUNCH_1 )   mapVal = CTRL_BT_LAUNCH_1  ;
    else if ( ev->data.control.param == FORCE_BT_LAUNCH_2 )   mapVal = CTRL_BT_LAUNCH_2  ;
    else if ( ev->data.control.param == FORCE_BT_LAUNCH_3 )   mapVal = CTRL_BT_LAUNCH_3  ;
    else if ( ev->data.control.param == FORCE_BT_LAUNCH_4 )   mapVal = CTRL_BT_LAUNCH_4  ;
    else if ( ev->data.control.param == FORCE_BT_LAUNCH_5 )   mapVal = CTRL_BT_LAUNCH_5  ;
    else if ( ev->data.control.param == FORCE_BT_LAUNCH_6 )   mapVal = CTRL_BT_LAUNCH_6  ;
    else if ( ev->data.control.param == FORCE_BT_LAUNCH_7 )   mapVal = CTRL_BT_LAUNCH_7  ;
    //else if ( ev->data.control.param == FORCE_BT_LAUNCH_8 )   mapVal = CTRL_BT_LAUNCH_8  ;

    else if ( ev->data.control.param == FORCE_BT_RIGHT )  mapVal = CTRL_BT_RIGHT    ;
    else if ( ev->data.control.param == FORCE_BT_LEFT )   mapVal = CTRL_BT_LEFT   ;
    else if ( ev->data.control.param == FORCE_BT_UP )      ;
    else if ( ev->data.control.param == FORCE_BT_DOWN )   mapVal = CTRL_BT_DOWN ;

    else if ( ev->data.control.param == FORCE_BT_MUTE )   {
      if ( ev->data.control.value == 3 ) {
        mapVal = CTRL_BT_LAUNCH_8   ;
  
        // Set color of "Stop Solo Mode button"
        mapVal2 = CTRL_COLOR_AMBER ;
      }
    }

    else if ( ev->data.control.param == FORCE_BT_SOLO )   {
      if ( ev->data.control.value == 3 ) {
        mapVal = CTRL_BT_LAUNCH_8   ;
   
        // Set color of "Stop Solo Mode button"
        mapVal2 = CTRL_COLOR_BLUE ;
      }
    }

    else if ( ev->data.control.param == FORCE_BT_REC_ARM ) {
      if ( ev->data.control.value == 3 ) {
        mapVal = CTRL_BT_LAUNCH_8   ;

        // Set color of "Stop Solo Mode button"
        mapVal2 = CTRL_COLOR_RED ;
      }
    }

    else if ( ev->data.control.param == FORCE_BT_CLIP_STOP )   {
      if ( ev->data.control.value == 3 ) {
        mapVal = CTRL_BT_LAUNCH_8   ;
   
        // Set color of "Stop Solo Mode button"
        mapVal2 = CTRL_COLOR_GREEN ;
      }
    }

    if ( mapVal >=0 ) {
        snd_seq_event_t ev2 = *ev;
        ev2.data.control.param = mapVal;
        if ( mapVal2 >= 0 ) ev2.data.control.value = mapVal2 ;
        else {
          uint8_t colMap[]={0x75,0x01,0X13,0x29,0x57};
          ev2.data.control.value = colMap[ev->data.control.value] ;
        }

        SetMidiEventDestination(&ev2, TO_CTRL_EXT );
        SendMidiEvent(&ev2 );
    }

}

///////////////////////////////////////////////////////////////////////////////
// Refresh columns mode lines 7 & 8 on the LaunchPad
///////////////////////////////////////////////////////////////////////////////
static void ControllerRefreshColumnsPads(bool show) {
  if ( show ) {
    // Line 9 = track selection
    // Line 8 = mute modes
    ControllerDrawPadsLineFromForceCache(9,1);
    ControllerDrawPadsLineFromForceCache(8,0);
  }
  else {
    ControllerDrawPadsLineFromForceCache(6,1);
    ControllerDrawPadsLineFromForceCache(7,0);
  }
}

///////////////////////////////////////////////////////////////////////////////
// Process an event received from the Launchpad
///////////////////////////////////////////////////////////////////////////////
static bool ControllerEventReceived(snd_seq_event_t *ev) {

  switch (ev->type) {
    case SND_SEQ_EVENT_SYSEX:
      break;

    case SND_SEQ_EVENT_CONTROLLER:
      // Buttons around Launchpad mini pads
      if ( ev->data.control.channel == 0 ) {
        int mapVal = -1;

        if       ( ev->data.control.param == CTRL_BT_UP) { // ^
          // UP holded = shitfmode on the Launchpad
          CtrlShiftMode = ( ev->data.control.value == 0x7F );
 
          // If we press shift while holding launch 8/SSM,  that will trig Record Arm
          // only in that sequence !
          if ( ControllerColumnsPadMode && CtrlShiftMode ) {
                CurrentSoloMode = FORCE_SM_REC_ARM;
                mapVal = SoloModeButtonMap[CurrentSoloMode];
          } 
          else 
          return false;
        }

        else if  ( ev->data.control.param == CTRL_BT_LAUNCH_8  ) { // v
          // Launch 8 button  to manage columns pads

          // Shift is used to launch the 8th line
          if ( CtrlShiftMode )           mapVal = FORCE_BT_LAUNCH_8;
          else {

            ControllerColumnsPadMode = ( ev->data.control.value == 0x7F ) || ControllerColumnsPadModeLocked ;
            ControllerRefreshColumnsPads(ControllerColumnsPadMode);
            return false;
          }
        }

        else if  ( ev->data.control.param == CTRL_BT_DOWN  ) { // v
          // Shift down = UP
          mapVal = CtrlShiftMode ? FORCE_BT_UP : FORCE_BT_DOWN ;
        }
        else if  ( ev->data.control.param == CTRL_BT_LEFT ) { // <

          mapVal = CtrlShiftMode ? FORCE_BT_LEFT : FORCE_BT_COPY  ;
        }
        else if  ( ev->data.control.param == CTRL_BT_RIGHT ) { // >
          mapVal = CtrlShiftMode ? FORCE_BT_RIGHT :FORCE_BT_DELETE   ;
        }
        // Launch buttons
        else if  ( ev->data.control.param == CTRL_BT_LAUNCH_1 ) { // >
          mapVal = FORCE_BT_LAUNCH_1 ;
        }
        else if  ( ev->data.control.param == CTRL_BT_LAUNCH_2 ) { // >
          mapVal = FORCE_BT_LAUNCH_2 ;
        }
        else if  ( ev->data.control.param == CTRL_BT_LAUNCH_3 ) { // >
          // Launch 3 generates a FORCE SHIFT FOR columns options setting
          // in columns pads mode
          mapVal = ControllerColumnsPadMode ? FORCE_BT_SHIFT : FORCE_BT_LAUNCH_3 ;
        }

        else if  ( ev->data.control.param == CTRL_BT_LAUNCH_4 ) { // >
          if ( ControllerColumnsPadMode  ) {
            // Lock Columns pads mode
            if ( ev->data.control.value !=0 )  ControllerColumnsPadModeLocked = ! ControllerColumnsPadModeLocked;
            return false;
          }
          mapVal = FORCE_BT_LAUNCH_4 ;
        }

        // Launch 5 / Stop clip
        else if  ( ev->data.control.param == CTRL_BT_LAUNCH_5 ) { // >

          if ( ControllerColumnsPadMode ) {
            if ( ev->data.control.value != 0 ) {
              CurrentSoloMode = FORCE_SM_CLIP_STOP;
            }
            mapVal = SoloModeButtonMap[CurrentSoloMode];
          }
          else {
            mapVal = FORCE_BT_LAUNCH_5;
          }
        }

        // Launch 6 / Mute
        else if  ( ev->data.control.param == CTRL_BT_LAUNCH_6 ) { // >

          if ( ControllerColumnsPadMode ) {
            if ( ev->data.control.value != 0 ) {
              CurrentSoloMode = FORCE_SM_MUTE;
            }
            mapVal = SoloModeButtonMap[CurrentSoloMode];
          }
          else {
            mapVal = FORCE_BT_LAUNCH_6;
          }
        }

        // Launch 7 / Solo
        else if  ( ev->data.control.param == CTRL_BT_LAUNCH_7 ) { // >

          if ( ControllerColumnsPadMode ) {
            if ( ev->data.control.value != 0 ) {
              CurrentSoloMode = FORCE_SM_SOLO;
            }
            mapVal = SoloModeButtonMap[CurrentSoloMode];
          }
          else {
            mapVal = FORCE_BT_LAUNCH_7;
          }
        }


        else if  ( ev->data.control.param == CTRL_BT_SESSION ) { // >
          mapVal = CtrlShiftMode ? FORCE_BT_MASTER : FORCE_BT_LAUNCH ;
        }

        else if  ( ev->data.control.param == CTRL_BT_USER1 ) { // >
          mapVal = FORCE_BT_STEP_SEQ ;
        }

        else if  ( ev->data.control.param == CTRL_BT_USER2 ) { // >
          mapVal = FORCE_BT_NOTE ;
        }

        else if  ( ev->data.control.param == CTRL_BT_MIXER ) { // >
          mapVal = FORCE_BT_MIXER ;
        }

        if ( mapVal >= 0 ) {

            SendDeviceKeyEvent(mapVal,ev->data.control.value);

            return false;

        }

//        dump_event(ev);

      }
      break;

    case SND_SEQ_EVENT_NOTEON:
    case SND_SEQ_EVENT_NOTEOFF:
    case SND_SEQ_EVENT_KEYPRESS:
      if ( ev->data.note.channel == 0 ) {
        // Launchpad mini pads
        ev->data.note.note = ControllerGetForcePadNote(ev->data.note.note);
        SetMidiEventDestination(ev,TO_MPC_PRIVATE );
        // If controller column mode, simulate the columns and mutes pads
        if ( ControllerColumnsPadMode) {

            if ( ev->type == SND_SEQ_EVENT_KEYPRESS ) return false;

            uint8_t padFL = (ev->data.note.note - FORCEPADS_NOTE_OFFSET ) / 8 ;
            uint8_t padFC = ( ev->data.note.note - FORCEPADS_NOTE_OFFSET ) % 8 ;

            // Edit current track
            if ( padFL == 0 ) {

              ev->data.note.note = FORCE_BT_COLUMN_PAD1 + padFC ;
              ev->data.note.velocity = ( ev->data.note.velocity > 0 ? 0x7F:00);
              
              if ( ev->data.note.velocity == 0x7F ) {
                SendDeviceKeyEvent(FORCE_BT_EDIT, 0x7F) ; // Send Edit Press
                SendMidiEvent(ev); // Send Pad On
                ev->data.note.velocity = 0 ;
                SendMidiEvent(ev); // Send Pad off
              }
              else SendDeviceKeyEvent(FORCE_BT_EDIT, 0); // Send Edit Off

              return false;
            }

            else if ( padFL == 6 ) { // Column pads (on the mini launchpad)
              ev->data.note.note = FORCE_BT_COLUMN_PAD1 + padFC;
              ev->data.note.velocity = ( ev->data.note.velocity > 0 ? 0x7F:00);

            }
            else if ( padFL == 7 ) { // Mute mode pads (on the mini lLaunchpad)
              ev->data.note.note = FORCE_BT_MUTE_PAD1 + padFC;
              ev->data.note.velocity = ( ev->data.note.velocity > 0 ? 0x7F:00);
            }
        }
        else { 
          ev->data.note.channel = 9; // Note Force pad
                // If Shift Mode, simulate Select key
          if ( CtrlShiftMode ) {
            SendDeviceKeyEvent(FORCE_BT_SELECT,0x7F);
            SendMidiEvent(ev);
            SendDeviceKeyEvent(FORCE_BT_SELECT,0);
            return false;
          }
        }
      }
  }

  return true;
}
