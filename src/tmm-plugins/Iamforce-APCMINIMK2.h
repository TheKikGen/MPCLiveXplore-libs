/*
__ __| |           |  /_) |     ___|             |           |
  |   __ \   _ \  ' /  | |  / |      _ \ __ \   |      _` | __ \   __|
  |   | | |  __/  . \  |   <  |   |  __/ |   |  |     (   | |   |\__ \
 _|  _| |_|\___| _|\_\_|_|\_\\____|\___|_|  _| _____|\__,_|_.__/ ____/
-----------------------------------------------------------------------------
TKGL_MIDIMAPPER  custom mapping library .
This is a custom midi mapping library for TKGL_MIDIMAPPER LD_PRELOAD library

----------------------------------------------------------------------------
AKAI APC MINI MK2  FOR IAMFORCE
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

#define IAMFORCE_DRIVER_VERSION "1.0"
#define IAMFORCE_DRIVER_ID "APCMINIMK2"
#define IAMFORCE_DRIVER_NAME "Akai APC Mini mk2"
#define IAMFORCE_ALSASEQ_DEFAULT_CLIENT_NAME "APC Mini mk2"
#define IAMFORCE_ALSASEQ_DEFAULT_PORT 1


// DEBUG
//#define SX_APCK_DEVICE_ID 0x4E
//#define IAMFORCE_ALSASEQ_DEFAULT_CLIENT_NAME "APC Key 25 mk2"

// SYSEX

#define SX_APCK_DEVICE_ID 0x4F
#define SX_APCK_HEADER 0xF0,0x47,0x7F,SX_APCK_DEVICE_ID

// LED RGB COLOR
// Sysex header
//     0x24 MSB (data size) LSB (data size)
//     (start pad) (end pad)
//     (red brigthness MSB) (red brigthness LSB)
//     (green brigthness MSB) (green brigthness LSB)
//     (blue brigthness MSB) (blue brigthness LSB)
// 0xF7
// NB : Due to a bug in th APC Key 25 mk2, the pad part of the sysex
// msg from the pad from-to must be duplicated.

uint8_t SX_APCK_LED_RGB_COLOR[] = {
  SX_APCK_HEADER, 0x24,
  0x00, 0x10,
  0x00, 0x00, 0x00, 0X00, 0x00, 0x00, 0x00, 0X00,
  0x00, 0x00, 0x00, 0X00, 0x00, 0x00, 0x00, 0X00,
  0xF7 };

// Buttons
#define CTRL_BT_TRACK_1 0x64
#define CTRL_BT_TRACK_2 0x65
#define CTRL_BT_TRACK_3 0x66
#define CTRL_BT_TRACK_4 0x67
#define CTRL_BT_TRACK_5 0x68
#define CTRL_BT_TRACK_6 0x69
#define CTRL_BT_TRACK_7 0x6A
#define CTRL_BT_TRACK_8 0x6B

#define CTRL_BT_LAUNCH_1 0x70
#define CTRL_BT_LAUNCH_2 0x71
#define CTRL_BT_LAUNCH_3 0x72
#define CTRL_BT_LAUNCH_4 0x73
#define CTRL_BT_LAUNCH_5 0x74
#define CTRL_BT_LAUNCH_6 0x75
#define CTRL_BT_LAUNCH_7 0x76
#define CTRL_BT_LAUNCH_8 0x77

#define CTRL_BT_SHIFT 0x7A

// Faders - Absolute
#define CTRL_FADER_1 0x30
#define CTRL_FADER_2 0x31
#define CTRL_FADER_3 0x32
#define CTRL_FADER_4 0x33
#define CTRL_FADER_5 0x34
#define CTRL_FADER_6 0x35
#define CTRL_FADER_7 0x36
#define CTRL_FADER_8 0x37
#define CTRL_FADER_9 0x38

// Pads start at 0 from bottom left to upper right
#define CTRL_PAD_NOTE_OFFSET 0x27
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

///////////////////////////////////////////////////////////////////////////////
// APC Key 25 Set several pads RGB Colors
///////////////////////////////////////////////////////////////////////////////
static void ControllerSetMultiPadsColorRGB(uint8_t padCtFrom, uint8_t padCtTo, uint8_t r, uint8_t g, uint8_t b) {

  //  F0 47 7F 4E 24
  //  00 10
  //  00 00 00 7F 00 00 00 00
  //  00 00 00 7F 00 00 00 00
  //  F7

  // APC key colors range is 0 - 255. Force is 0 - 127
  r <<= 1 ;  g <<= 1 ; b <<=  1;

  SX_APCK_LED_RGB_COLOR[7]  = padCtFrom;
  SX_APCK_LED_RGB_COLOR[8]  = padCtTo;

  SX_APCK_LED_RGB_COLOR[9]  = r  >> 7  ;
  SX_APCK_LED_RGB_COLOR[10]  = r & 0x7F;

  SX_APCK_LED_RGB_COLOR[11]  = g  >> 7  ;
  SX_APCK_LED_RGB_COLOR[12]  = g & 0x7F;

  SX_APCK_LED_RGB_COLOR[13]  = b  >> 7  ;
  SX_APCK_LED_RGB_COLOR[14]  = b & 0x7F;

  // Firmware bug workaround
  memcpy( &SX_APCK_LED_RGB_COLOR[15],&SX_APCK_LED_RGB_COLOR[7],8 );

  SeqSendRawMidi( TO_CTRL_EXT,SX_APCK_LED_RGB_COLOR,sizeof(SX_APCK_LED_RGB_COLOR) );
}

///////////////////////////////////////////////////////////////////////////////
// APC Key 25 fill the matrix with a RGB color
///////////////////////////////////////////////////////////////////////////////
static void ControllerFillPadMatrixColorRGB(uint8_t r, uint8_t g, uint8_t b) {
  ControllerSetMultiPadsColorRGB(0, CTRL_PAD_NOTE_OFFSET, r,  g,  b) ;
}

///////////////////////////////////////////////////////////////////////////////
// APC Key 25 Set a pad RGB Colors
///////////////////////////////////////////////////////////////////////////////
static void ControllerSetPadColorRGB(uint8_t padCt, uint8_t r, uint8_t g, uint8_t b) {
  ControllerSetMultiPadsColorRGB(padCt, padCt, r,  g,  b) ;
}

///////////////////////////////////////////////////////////////////////////////
// Show a line from Force pad color cache (source = Force pad#, Dest = ctrl pad #)
///////////////////////////////////////////////////////////////////////////////
static void ControllerDrawPadsLineFromForceCache(uint8_t SrcForce, uint8_t DestCtrl) {

    if ( DestCtrl >= CTRL_PAD_MAX_LINE ) return;

    uint8_t pf = SrcForce * 8 ;
    uint8_t pl = DestCtrl * CTRL_PAD_MAX_COL ;

    for ( uint8_t i = 0  ; i <  8 ; i++) {
      ControllerSetPadColorRGB(pl, Force_PadColorsCache[pf].c.r, Force_PadColorsCache[pf].c.g,Force_PadColorsCache[pf].c.b);
      pf++; pl++;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Refresh the pad surface from Force pad cache within a quadran
///////////////////////////////////////////////////////////////////////////////
static void ControllerRefreshMatrixFromForceCache() {

    int padCt = -1;
    for ( int i = CtrlPadQuadran ; i< 64 ; i++) {
      padCt = ControllerGetPadIndex(i - CtrlPadQuadran) ;
      //tklog_debug("RefreshMatrix Quadran %d padF %d padCt %d\n",CtrlPadQuadran,i,padCt );
      if ( padCt >= 0 ) ControllerSetPadColorRGB(ControllerGetPadIndex(i - CtrlPadQuadran),  Force_PadColorsCache[i].c.r, Force_PadColorsCache[i].c.g,Force_PadColorsCache[i].c.b);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Return a ctrl led value from Force Launch button led
///////////////////////////////////////////////////////////////////////////////
static uint8_t ControllerGetLaunchLedValue(uint8_t ledF) {

// Force led : 
// 0 : Off  1-0x7F : Fixed on   2 : Blink
  switch (ledF)
  {
    case 0:  
      break;
    case 3: 
    case 4:  
      ledF = 2;    
      break;  
    default:
      ledF = 1;    
      break;
  }

  return ledF;
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
    // Restore initial pads color, but take care of the current quadran...
    ControllerDrawPadsLineFromForceCache( 6,1);
    ControllerDrawPadsLineFromForceCache( 7,0);
  }
}

///////////////////////////////////////////////////////////////////////////////
// Controller initialization
///////////////////////////////////////////////////////////////////////////////
static int ControllerInitialize() {
  tklog_info("IamForce : %s implementation, version %s.\n",IAMFORCE_DRIVER_NAME,IAMFORCE_DRIVER_VERSION);
  ControllerFillPadMatrixColorRGB(0x7F,0,0);
}

///////////////////////////////////////////////////////////////////////////////
// Get a Force pad index from an APC Key index
///////////////////////////////////////////////////////////////////////////////
static uint8_t ControllerGetForcePadIndex(uint8_t padCt) {
  // Convert pad to Force pad #
  uint8_t padC  =  padCt  %  CTRL_PAD_MAX_COL ;
  uint8_t padL  =  CTRL_PAD_MAX_LINE - 1 - padCt / CTRL_PAD_MAX_COL ;
  return   padL * 8 + padC ;
}

///////////////////////////////////////////////////////////////////////////////
// Get a Force pad note from an APC Key index
///////////////////////////////////////////////////////////////////////////////
static uint8_t ControllerGetForcePadNote(uint8_t padCt) {
  return   ControllerGetForcePadIndex(padCt) + FORCEPADS_NOTE_OFFSET;
}

///////////////////////////////////////////////////////////////////////////////
// Get a APC Key Controller pad index from a Force pad index
///////////////////////////////////////////////////////////////////////////////
static int ControllerGetPadIndex(uint8_t padF) {

  if ( padF >= 64 ) return -1;

  return  ( CTRL_PAD_MAX_LINE - 1 - padF / 8 ) * CTRL_PAD_MAX_COL + padF % 8 ;
}

///////////////////////////////////////////////////////////////////////////////
// Map LED lightning messages from Force with Launchpad buttons leds colors
///////////////////////////////////////////////////////////////////////////////
static void ControllerSetMapButtonLed(snd_seq_event_t *ev) {

    int mapVal = -1 ;
    int mapVal2 = -1;

    //if ( ev->data.control.param != FORCE_BT_TAP_TEMPO)  tklog_debug("LED VALUE  %02X = %02X\n",ev->data.control.param,ev->data.control.value);   

    if      ( ev->data.control.param >= FORCE_BT_LAUNCH_1  && ev->data.control.param <= FORCE_BT_LAUNCH_8 )   {

      //tklog_debug("LED VALUE  %02X = %02X\n",ev->data.control.param,ev->data.control.value);   

      mapVal2 = ControllerGetLaunchLedValue(ev->data.control.value);
      uint8_t b = ev->data.control.param - FORCE_BT_LAUNCH_1;
      uint8_t ql = CtrlPadQuadran / 8;
      
      if ( b >= ql ) {
        
        mapVal = CTRL_BT_LAUNCH_1 + b - ql ; 

      }
      else return;
    }

    else if ( ev->data.control.param == FORCE_BT_MIXER )  {
      mapVal = CTRL_BT_TRACK_1 ;
      mapVal2 =  ev->data.control.value == 3 ? 0x7F:00;
    }

    else if ( ev->data.control.param == FORCE_BT_LAUNCH )  {
      mapVal = CTRL_BT_TRACK_2 ;
      mapVal2 =  ev->data.control.value == 3 ? 0x7F:00;
    }

    else if ( ev->data.control.param == FORCE_BT_NOTE )  {
      mapVal = CTRL_BT_TRACK_3 ;
      mapVal2 =  ev->data.control.value == 3 ? 0x7F:00;
    }

    else if ( ev->data.control.param == FORCE_BT_MATRIX )  {
      mapVal = CTRL_BT_TRACK_4 ;
      mapVal2 =  ev->data.control.value == 3 ? 0x7F:00;
    }

    else if ( ev->data.control.param == FORCE_BT_COPY )   {
      // LED : 0 = off, 1 low bright, 3 high bright
      mapVal  = CTRL_BT_TRACK_5;
      mapVal2 =  ev->data.control.value == 3 ? 0x7F:00 ;
    }   

    else if ( ev->data.control.param == FORCE_BT_DELETE )  {
      mapVal = CTRL_BT_TRACK_6 ;
      mapVal2 =  ev->data.control.value == 3 ? 0x7F:00;
    }

    else if ( ev->data.control.param == FORCE_BT_EDIT )  {
      mapVal = CTRL_BT_TRACK_7 ;
      mapVal2 =  ev->data.control.value == 3 ? 0x7F:00;
    }
    
    else if ( ev->data.control.param == FORCE_BT_STEP_SEQ )  {
      mapVal = CTRL_BT_TRACK_8 ;
      mapVal2 =  ev->data.control.value == 3 ? 0x7F:00;
    }


    else if ( ev->data.control.param == FORCE_BT_MUTE )   {
      if ( ev->data.control.value == 3 ) {
        CurrentSoloMode = FORCE_SM_MUTE ; // Resynchronize
      }
    }

    else if ( ev->data.control.param == FORCE_BT_SOLO )   {
      if ( ev->data.control.value == 3 ) {
        CurrentSoloMode = FORCE_SM_SOLO ; // Resynchronize
      }
    }

    else if ( ev->data.control.param == FORCE_BT_REC_ARM ) {
      if ( ev->data.control.value == 3 ) {
        CurrentSoloMode = FORCE_SM_REC_ARM ; // Resynchronize
      }
    }

    else if ( ev->data.control.param == FORCE_BT_CLIP_STOP )   {
      if ( ev->data.control.value == 3 ) {
        CurrentSoloMode = FORCE_SM_CLIP_STOP ; // Resynchronize
      }
    }

    if ( mapVal >=0 ) {
        // Send a LED message to the APC Key
        snd_seq_event_t ev2 = *ev;
        ev2.type = SND_SEQ_EVENT_NOTEON;
        ev2.data.note.channel = 0;
        ev2.data.note.note = mapVal;
        ev2.data.note.velocity = mapVal2 >= 0 ? mapVal2:ev->data.control.value;

        SetMidiEventDestination(&ev2, TO_CTRL_EXT );
        SendMidiEvent(&ev2 );
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
      if ( ev->data.control.channel == 0 ) {

        // Knob 1 - 8
        // if ( ev->data.control.param >= CTRL_KNOB_1 && ev->data.control.param  <= CTRL_KNOB_8 ) {

        //   static uint8_t lastKnob = 0xFF;

        //   SetMidiEventDestination(ev,TO_MPC_PRIVATE );

        //   uint8_t k = ev->data.control.param - CTRL_KNOB_1 ;
        //   //tklog_debug("Knob no %d\n",k);

        //   // Remap controller
        //   ev->data.control.param = FORCE_KN_QLINK_1 + k;
        //   //tklog_debug("Knob remap %02x\n",ev->data.control.param);

        //   if ( lastKnob != k  ) {
        //     snd_seq_event_t ev2 = *ev;
        //     ev2.data.note.channel = 0;
        //     SetMidiEventDestination(&ev2,TO_MPC_PRIVATE );

        //     if ( lastKnob != 0xFF) {
        //       // Simulate an "untouch", but not the first time
        //       ev2.type = SND_SEQ_EVENT_NOTEOFF;
        //       ev2.data.note.velocity = 0x00;
        //       ev2.data.note.note = FORCE_BT_QLINK1_TOUCH + lastKnob ;
        //       SendMidiEvent(&ev2 );
        //     }

        //     // Simulate a "touch" knob
        //     ev2.type = SND_SEQ_EVENT_NOTEON;
        //     ev2.data.note.velocity = 0x7F;
        //     ev2.data.note.note = FORCE_BT_QLINK1_TOUCH + k ;
        //     SendMidiEvent(&ev2 );
        //   }

        //  lastKnob = k;
        }
      break;

    case SND_SEQ_EVENT_NOTEON:
    case SND_SEQ_EVENT_NOTEOFF:
    case SND_SEQ_EVENT_KEYPRESS:

      // Route to MPC app by default
      SetMidiEventDestination(ev,TO_MPC_PRIVATE );

      if ( ev->data.note.channel == 0 ) {

        if ( ev->data.note.note > CTRL_PAD_NOTE_OFFSET && ev->type != SND_SEQ_EVENT_KEYPRESS ) {
          // Buttons
          // NB : Release is a note off (0x80)  + velocity zero. We test nonly velocity here.
          int mapVal = -1;

          if       ( ev->data.note.note == CTRL_BT_SHIFT) {
            CtrlShiftMode = ( ev->data.note.velocity == 0x7F );
            ColumnsPadMode = CtrlShiftMode;
            ControllerRefreshColumnsPads(ColumnsPadMode);

            return false;
          }

          // UP / COPY 
          else if  ( ev->data.note.note == CTRL_BT_TRACK_5  ) {
            mapVal = CtrlShiftMode ? FORCE_BT_UP : FORCE_BT_COPY  ;
          }

          // DOWN / DELETE
          else if  ( ev->data.note.note == CTRL_BT_TRACK_6  ) {
            mapVal = CtrlShiftMode ? FORCE_BT_DOWN :FORCE_BT_DELETE ;
          }
          // LEFT / EDIT 
          else if  ( ev->data.note.note == CTRL_BT_TRACK_7  ) {
            mapVal = CtrlShiftMode ? FORCE_BT_LEFT : FORCE_BT_EDIT  ;
          }

          // Right / Select  
          else if  ( ev->data.note.note == CTRL_BT_TRACK_8  ) {
            mapVal = CtrlShiftMode ? FORCE_BT_RIGHT :FORCE_BT_SELECT ;
          }

          // Volume / Mixer / Master
          else if  ( ev->data.note.note == CTRL_BT_TRACK_1  ) {
            mapVal = CtrlShiftMode ? FORCE_BT_MASTER : FORCE_BT_MIXER ;
          }

          // Pan / Assign A / Assign B
          else if  ( ev->data.note.note == CTRL_BT_TRACK_2  ) {
             mapVal = CtrlShiftMode ? FORCE_BT_ASSIGN_B : FORCE_BT_ASSIGN_A ;
          }

          // Send = Launch
          else if  ( ev->data.note.note == CTRL_BT_TRACK_3  ) {
             
             mapVal = FORCE_BT_LAUNCH ;
          }

          //  Device / Menu / Matrix
          else if  ( ev->data.note.note == CTRL_BT_TRACK_4  ) {
             mapVal = CtrlShiftMode ? FORCE_BT_MENU:FORCE_BT_MATRIX ;
          }

          // Launch 1 / Clip Stop
          else if  ( ev->data.note.note == CTRL_BT_LAUNCH_1  ) {
            if ( ColumnsPadMode ) {
              CurrentSoloMode = FORCE_SM_CLIP_STOP ;
              mapVal = SoloModeButtonMap[CurrentSoloMode];
            }
            else  mapVal = FORCE_BT_LAUNCH_1 ;
          }

          // Launch 2 / Solo
          else if  ( ev->data.note.note == CTRL_BT_LAUNCH_2  ) {
            if ( ColumnsPadMode ) {
              CurrentSoloMode = FORCE_SM_SOLO ;
              mapVal = SoloModeButtonMap[CurrentSoloMode];
            }
            else  mapVal = FORCE_BT_LAUNCH_2 ;
          }

          // Launch 3 / Mute
          else if  ( ev->data.note.note == CTRL_BT_LAUNCH_3  ) {
            if ( ColumnsPadMode ) {
              CurrentSoloMode = FORCE_SM_MUTE ;
              mapVal = SoloModeButtonMap[CurrentSoloMode];
            }
            else  mapVal = FORCE_BT_LAUNCH_3 ;
          }

          // Launch 4 / REC ARM
          else if  ( ev->data.note.note == CTRL_BT_LAUNCH_4  ) {
            if ( ColumnsPadMode ) {
              CurrentSoloMode = FORCE_SM_REC_ARM ;
              mapVal = SoloModeButtonMap[CurrentSoloMode];
            }
            else  mapVal = FORCE_BT_LAUNCH_4 ;
          }

          // Launch 5 (Select) / Knobs select
          else if  ( ev->data.note.note == CTRL_BT_LAUNCH_5  ) {
            mapVal = ColumnsPadMode ? FORCE_BT_KNOBS : FORCE_BT_LAUNCH_5;
          }

          // Launch 6 (Drum) / 
          else if  ( ev->data.note.note == CTRL_BT_LAUNCH_6  ) {
            mapVal = ColumnsPadMode ? FORCE_BT_STEP_SEQ : FORCE_BT_LAUNCH_6;
          }

          // Launch 7 (Note) / 
          else if  ( ev->data.note.note == CTRL_BT_LAUNCH_7  ) {
            mapVal = ColumnsPadMode ? FORCE_BT_NOTE : FORCE_BT_LAUNCH_7;
          }

          // Launch 8 (stop all) / 
          else if  ( ev->data.note.note == CTRL_BT_LAUNCH_8  ) {
            mapVal = ColumnsPadMode ? FORCE_BT_STOP_ALL : FORCE_BT_LAUNCH_8;
          }

          if ( mapVal >= 0 ) {
                ev->data.note.note = mapVal;
                // Do note on even if note off as Force use only velocity and Note On
                ev->type = SND_SEQ_EVENT_NOTEON;
          }
        }
        else {
          // APC Key 25 pads below 0x28

          // If controller column mode, simulate the columns and mutes pads
          // and track edit with pads on line 0
          if ( ColumnsPadMode) {

              if ( ev->type == SND_SEQ_EVENT_KEYPRESS ) return false;

              uint8_t padF = ControllerGetForcePadIndex(ev->data.note.note);
              uint8_t padFL = ev->data.note.note  / 8 ;
              uint8_t padFC = ev->data.note.note  % 8 ;

              // Edit current track
              if ( padFL == CTRL_PAD_MAX_LINE - 1 ) {

                ev->data.note.note = FORCE_BT_COLUMN_PAD1 + padFC ;
                ev->data.note.velocity  = ( ev->data.note.velocity > 0 ? 0x7F:00);

                if ( ev->data.note.velocity == 0x7F ) {
                  SendDeviceKeyEvent(FORCE_BT_EDIT, 0x7F) ; // Send Edit Press
                  SendMidiEvent(ev); // Send Pad On
                  ev->data.note.velocity = 0 ;
                  SendMidiEvent(ev); // Send Pad off
                }
                else SendDeviceKeyEvent(FORCE_BT_EDIT, 0); // Send Edit Off

                return false;
                
              }

              else if ( padFL == 1 ) { // Column pads
                ev->data.note.note = FORCE_BT_COLUMN_PAD1 + padFC;
                ev->data.note.velocity == ( ev->data.note.velocity > 0 ? 0x7F:00);

              }
              else if ( padFL == 0 ) { // Mute mode pads
                ev->data.note.note = FORCE_BT_MUTE_PAD1 + padFC;
                ev->data.note.velocity == ( ev->data.note.velocity > 0 ? 0x7F:00);
              }
          }
          else
            {
              ev->data.note.channel = 9; // Simulate Note Force pad
              // Take car of current quadran
              ev->data.note.note = ControllerGetForcePadNote(ev->data.note.note) + CtrlPadQuadran;

            }
        }
      }
  }

  return true;
}
