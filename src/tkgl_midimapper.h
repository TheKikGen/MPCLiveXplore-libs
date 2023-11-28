/*
__ __| |           |  /_) |     ___|             |           |
  |   __ \   _ \  ' /  | |  / |      _ \ __ \   |      _` | __ \   __|
  |   | | |  __/  . \  |   <  |   |  __/ |   |  |     (   | |   |\__ \
 _|  _| |_|\___| _|\_\_|_|\_\\____|\___|_|  _| _____|\__,_|_.__/ ____/
-----------------------------------------------------------------------------
TKGL_MIDIMAPPER  LD_PRELOAD library.
This "low-level" library allows you to hijack the MPC/Force application to add
your own midi mapping to input and output midi messages.

-----------------------------------------------------------------------------

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


#define VERSION "BETA2"

#define TKGL_LOGO "\
__ __| |           |  /_) |     ___|             |           |\n\
  |   __ \\   _ \\  ' /  | |  / |      _ \\ __ \\   |      _` | __ \\   __|\n\
  |   | | |  __/  . \\  |   <  |   |  __/ |   |  |     (   | |   |\\__ \\\n\
 _|  _| |_|\\___| _|\\_\\_|_|\\_\\\\____|\\___|_|  _| _____|\\__,_|_.__/ ____/\n\
"

// Sysex

#define AKAI_DEVICE_SX_SET_PAD_RGB 0x65
#define AKAI_SYSEX_HEADER 0xF0,0x47, 0x7F
#define AKAI_SYSEX_IDENTITY_REPLY_HEADER 0xF0,0x7E,0x00,0x06,0x02,0x47

// Router
#define ROUTER_SEQ_NAME "TKGL Midi"

#define ROUTER_CTRL_PORT_NAME "Ctrl"

#define MIDI_DECODER_SIZE 256

#define TKROUTER_SEQ_POOL_OUT_SIZE 512

#define CTRL_MPC_ALL_PRIVATE "(MPC|Akai Pro Force|Force 6).*Privat.*$"

#define PRODUCT_CODE_PATH "/sys/firmware/devicetree/base/inmusic,product-code"
#define PRODUCT_COMPATIBLE_PATH "/sys/firmware/devicetree/base/compatible"
#define PRODUCT_COMPATIBLE_STR "inmusic,%sinmusic,az01rockchip,rk3288"

// Power supply faking
#define POWER_SUPPLY_ONLINE_PATH "/sys/class/power_supply/az01-ac-power/online"
#define POWER_SUPPLY_VOLTAGE_NOW_PATH "/sys/class/power_supply/az01-ac-power/voltage_now"
#define POWER_SUPPLY_PRESENT_PATH "/sys/class/power_supply/sbs-3-000b/present"
#define POWER_SUPPLY_STATUS_PATH "/sys/class/power_supply/sbs-3-000b/status"
#define POWER_SUPPLY_CAPACITY_PATH "/sys/class/power_supply/sbs-3-000b/capacity"

#define POWER_SUPPLY_ONLINE "1"
#define POWER_SUPPLY_VOLTAGE_NOW "18608000"
#define POWER_SUPPLY_PRESENT "1"
#define POWER_SUPPLY_STATUS "Full"
#define POWER_SUPPLY_CAPACITY "100"

// Colors R G B (nb . max r g b value is 7f / 7 bits  due to midi)

// Standard Force Colors
#define COLOR_FIRE       0x7F1919
#define COLOR_ORANGE     0x7F3F19
#define COLOR_TANGERINE  0x7F3419
#define COLOR_APRICOT    0x7F5019
#define COLOR_YELLOW     0x767617
#define COLOR_CANARY     0x7F6E19
#define COLOR_LEMON      0x757F19
#define COLOR_CHARTREUSE 0x5A7F19
#define COLOR_NEON       0x3B7F19
#define COLOR_LIME       0x207F19
#define COLOR_CLOVER     0x197F2E
#define COLOR_SEA        0x197F4C
#define COLOR_MINT       0x197F68
#define COLOR_CYAN       0x197C7F
#define COLOR_SKY        0x195D7F
#define COLOR_AZURE      0x19427F
#define COLOR_GREY       0x113D56
#define COLOR_MIDNIGHT   0x19277F
#define COLOR_INDIGO     0x3A197F
#define COLOR_VIOLET     0x46197F
#define COLOR_GRAPE      0x60197F
#define COLOR_FUSHIA     0x7F197F
#define COLOR_MAGENTA    0x7F1964
#define COLOR_CORAL      0x7F1949
#define COLOR_GREEN      0x125A39

// Bright colors
#define COLOR_WHITE        0x7F7F7F
#define COLOR_BLACK        0x000000
#define COLOR_FULL_RED     0x7F0000
#define COLOR_FULL_GREEN   0x007F00
#define COLOR_FULL_BLUE    0x00007F
#define COLOR_FULL_YELLOW  0x007F7F
#define COLOR_FULL_MAGENTA 0x7F007F

// Force buttons
#define FORCE_BT_ENCODER 0x6F
#define FORCE_BT_NAVIGATE 0
#define FORCE_BT_KNOBS 1
#define FORCE_BT_MENU 2
#define FORCE_BT_MATRIX 3
#define FORCE_BT_NOTE 4
#define FORCE_BT_MASTER 5
#define FORCE_BT_CLIP 9
#define FORCE_BT_MIXER 11
#define FORCE_BT_LOAD 35
#define FORCE_BT_SAVE 36
#define FORCE_BT_EDIT 37
#define FORCE_BT_DELETE 38
#define FORCE_BT_SHIFT 0x31
#define FORCE_BT_SELECT 52
#define FORCE_BT_TAP_TEMPO 0x35
#define FORCE_BT_PLUS 54
#define FORCE_BT_MINUS 55
#define FORCE_BT_LAUNCH_1 56
#define FORCE_BT_LAUNCH_2 57
#define FORCE_BT_LAUNCH_3 58
#define FORCE_BT_LAUNCH_4 59
#define FORCE_BT_LAUNCH_5 60
#define FORCE_BT_LAUNCH_6 61
#define FORCE_BT_LAUNCH_7 62
#define FORCE_BT_LAUNCH_8 63
#define FORCE_BT_UNDO 67
#define FORCE_BT_REC 73
#define FORCE_BT_STOP 81
#define FORCE_BT_PLAY 82
#define FORCE_BT_QLINK1_TOUCH 0x53
#define FORCE_BT_QLINK2_TOUCH 0X54
#define FORCE_BT_QLINK3_TOUCH 0x55
#define FORCE_BT_QLINK4_TOUCH 0x56
#define FORCE_BT_QLINK5_TOUCH 0x57
#define FORCE_BT_QLINK6_TOUCH 0x58
#define FORCE_BT_QLINK7_TOUCH 0x59
#define FORCE_BT_QLINK8_TOUCH 0x5A
#define FORCE_KN_QLINK_1 0x10
#define FORCE_KN_QLINK_2 0x11
#define FORCE_KN_QLINK_3 0x12
#define FORCE_KN_QLINK_4 0x13
#define FORCE_KN_QLINK_5 0x14
#define FORCE_KN_QLINK_6 0x15
#define FORCE_KN_QLINK_7 0x16
#define FORCE_KN_QLINK_8 0x17
#define FORCE_BT_STOP_ALL 95
#define FORCE_BT_UP 112
#define FORCE_BT_DOWN 113
#define FORCE_BT_LEFT 114
#define FORCE_BT_RIGHT 115
#define FORCE_BT_LAUNCH 116
#define FORCE_BT_STEP_SEQ 117
#define FORCE_BT_ARP 118
#define FORCE_BT_COPY 122
#define FORCE_BT_COLUMN_PAD1 96
#define FORCE_BT_COLUMN_PAD2 97
#define FORCE_BT_COLUMN_PAD3 98
#define FORCE_BT_COLUMN_PAD4 99
#define FORCE_BT_COLUMN_PAD5 100
#define FORCE_BT_COLUMN_PAD6 101
#define FORCE_BT_COLUMN_PAD7 102
#define FORCE_BT_COLUMN_PAD8 103
#define FORCE_BT_MUTE_PAD1 41
#define FORCE_BT_MUTE_PAD2 42
#define FORCE_BT_MUTE_PAD3 43
#define FORCE_BT_MUTE_PAD4 44
#define FORCE_BT_MUTE_PAD5 45
#define FORCE_BT_MUTE_PAD6 46
#define FORCE_BT_MUTE_PAD7 47
#define FORCE_BT_MUTE_PAD8 48
#define FORCE_BT_ASSIGN_A 123
#define FORCE_BT_ASSIGN_B 124
#define FORCE_BT_MUTE 91
#define FORCE_BT_SOLO 92
#define FORCE_BT_REC_ARM 93
#define FORCE_BT_CLIP_STOP 94

// MPC Buttons
#define MPC_BT_TC 0x7D
#define MPC_BT_QLINK_SELECT 0
#define MPC_BT_ENCODER 0x6F
#define MPC_BT_BANK_A 35
#define MPC_BT_BANK_B 36
#define MPC_BT_BANK_C 37
#define MPC_BT_BANK_D 38
#define MPC_BT_FULL_LEVEL 39
#define MPC_BT_16_LEVEL 40
#define MPC_BT_TRACK_MUTE 43
#define MPC_BT_NEXT_SEQ 42
#define MPC_BT_PROG_EDIT 02
#define MPC_BT_SAMPLE_EDIT 06
#define MPC_BT_PAD_MIXER 115
#define MPC_BT_CHANNEL_MIXER 116
#define MPC_BT_PROJECT 59
#define MPC_BT_PROGRAM 60
#define MPC_BT_PAD_SCENE 61
#define MPC_BT_PAD_PARAM 62
#define MPC_BT_SCREEN_CONTROL 63
#define MPC_BT_MENU 123
#define MPC_BT_BROWSE 50
#define MPC_BT_STEP_SEQ 41
#define MPC_BT_SAMPLER 125
#define MPC_BT_XYFX 126
#define MPC_BT_PAD_PERFORM 127
#define MPC_BT_REC_ARM 74
#define MPC_BT_READ_WRITE 75
#define MPC_BT_MUTE 76
#define MPC_BT_SOLO 77
#define MPC_BT_ERASE 9
#define MPC_BT_UNDO 67
#define MPC_BT_COPY 122
#define MPC_BT_F_KEY 78
#define MPC_BT_TAP_TEMPO 53
#define MPC_BT_MAIN 52
#define MPC_BT_KEY0 109
#define MPC_BT_KEY1 100
#define MPC_BT_KEY2 101
#define MPC_BT_KEY3 102
#define MPC_BT_KEY4 103
#define MPC_BT_KEY5 104
#define MPC_BT_KEY6 105
#define MPC_BT_KEY7 106
#define MPC_BT_KEY8 107
#define MPC_BT_KEY9 108
#define MPC_BT_ENTER 6F
#define MPC_BT_PLUS_MINUS 110
#define MPC_BT_MINUS 55
#define MPC_BT_PLUS 54
#define MPC_BT_NOTE_REPEAT 11
#define MPC_BT_SHIFT 49
#define MPC_BT_UP 56
#define MPC_BT_DOWN 57
#define MPC_BT_LEFT 65
#define MPC_BT_RIGHT 66
#define MPC_BT_CENTER 58
#define MPC_BT_LOCATE_BW 68
#define MPC_BT_LOCATE_FW 69
#define MPC_BT_LOCATE 	70
#define MPC_BT_LOCATE_FBW 71
#define MPC_BT_LOCATE_FFW 72
#define MPC_BT_REC 73
#define MPC_BT_STOP 81
#define MPC_BT_OVERDUB 80
#define MPC_BT_PLAY 82
#define MPC_BT_PLAY_START 83
#define MPC_BT_QLINK1_TOUCH 0x54
#define MPC_BT_QLINK2_TOUCH 0x55
#define MPC_BT_QLINK3_TOUCH 0x56
#define MPC_BT_QLINK4_TOUCH 0x57
#define MPC_BT_QLINK5_TOUCH 0x58
#define MPC_BT_QLINK6_TOUCH 0x59
#define MPC_BT_QLINK7_TOUCH 0x5A
#define MPC_BT_QLINK8_TOUCH 0x5B
#define MPC_BT_QLINK9_TOUCH 0x5C
#define MPC_BT_QLINK10_TOUCH 0x5D
#define MPC_BT_QLINK11_TOUCH 0x5E
#define MPC_BT_QLINK12_TOUCH 0x5F
#define MPC_BT_QLINK13_TOUCH 0x60
#define MPC_BT_QLINK14_TOUCH 0x61
#define MPC_BT_QLINK15_TOUCH 0x62
#define MPC_BT_QLINK16_TOUCH 0x63
#define MPC_BT_QLINK_SELECT_LED_1 0x5A
#define MPC_BT_QLINK_SELECT_LED_2 0x5B
#define MPC_BT_QLINK_SELECT_LED_3 0x5C
#define MPC_BT_QLINK_SELECT_LED_4 0x5D

// MPC PAd note values
#define MPC_PAD_1 0x25
#define MPC_PAD_2 0x24
#define MPC_PAD_3 0x2A
#define MPC_PAD_4 0x52
#define MPC_PAD_5 0x28
#define MPC_PAD_6 0x26
#define MPC_PAD_7 0x2E
#define MPC_PAD_8 0x2C
#define MPC_PAD_9 0x30
#define MPC_PAD_10 0x2F
#define MPC_PAD_11 0x2D
#define MPC_PAD_12 0x2B
#define MPC_PAD_13 0x31
#define MPC_PAD_14 0x37
#define MPC_PAD_15 0x33
#define MPC_PAD_16 0x35

// Mapping tables index offset
//#define MPCPADS_TABLE_IDX_OFFSET 0X24
#define FORCEPADS_NOTE_OFFSET 0X36

// Send and destination Ids for midi messages
enum FromPortsIds  {  FROM_MPC_PRIVATE, FROM_MPC_PUBLIC,FROM_CTRL_MPC, FROM_MPC_EXTCTRL, FROM_CTRL_EXT };
enum ToPortsIds    {  TO_CTRL_MPC_PRIVATE, TO_CTRL_MPC_PUBLIC, TO_MPC_PRIVATE, TO_MPC_EXTCTRL, TO_CTRL_EXT };

// Rbg color struct
typedef union
{
  uint32_t v;
  struct {
    uint8_t b : 8;
    uint8_t g : 8;
    uint8_t r : 8;
    uint8_t : 8;
  } c;
} RGBcolor_t;

// Device info block typedef
typedef struct {
  char    * productCode;
  char    * productCompatible;
  bool      hasBattery;
  uint8_t   sysexId;
  char    * productString;
  char    * productStringShort;
  uint8_t   qlinkKnobsCount;
  uint8_t   sysexIdReply[7];
} DeviceInfo_t;

// Routing ports
typedef struct {
  // Mpc surface controller
  struct {
    int card;
    int cli;
    int portPriv;
    int portPub;
  } MpcHW ;

  // Tkgl virtual ports exposing MPC rawmidi app ports
  // Port is always 0
  struct  {
    int cliPrivOut;
    int cliPrivIn;
    int cliPubOut;
  } VirtRaw;

  // External midi controller
  struct {
    int card;
    int cli;
    int port;
  } Ctrl;

  // MPC application External midi controller mirroring port
  // MPC apps needs different IN and OUT port #
  // Connection to portAppCtrl
  struct {
    int cli;
    int portIn;
    int portOut;
  } MPCCtrl;

  // Router ports. Mirror of physical/app ports
  int cli;
  int portPriv; // Raw virtual
  int portPub; // Raw
  int portAppCtrl; // MPC App to ctrl
  int portCtrl; // Hardware
  int portMpcPriv; // Hardware
  int portMpcPub;  // Hardware

  // Alsa sequencer and parser
  snd_seq_t *seq;

} TkRouter_t;

// Internal MPC product sysex id  ( a struct may be better....)
enum MPCIds  {  MPC_X,   MPC_LIVE,   MPC_FORCE, MPC_ONE,   MPC_LIVE_MK2, MPC_KEY_61,  MPC_XL, MPC_ONE_MK2, _END_MPCID };

// Function prototypes ---------------------------------------------------------

void ShowBufferHexDump(const uint8_t* data, ssize_t sz, uint8_t nl);
int match(const char *string, const char *pattern);
void dump_event(const snd_seq_event_t *ev);
int SeqSendRawMidi( uint8_t destId,  const uint8_t *buffer, size_t size ) ;
int SetMidiEventDestination(snd_seq_event_t *ev, uint8_t destId );
int SendMidiEvent(snd_seq_event_t *ev );
int GetSeqPortFromDestinationId(uint8_t destId );
void SendDeviceKeyEvent(uint8_t key,uint8_t value);
void SendDeviceKeyPress(uint8_t key);
static int ControllerGetPadIndex(uint8_t padF) ;
void DeviceSetPadColorRGB(const uint8_t mpcId, const uint8_t padNumber, const uint8_t r,const uint8_t g,const uint8_t b ) ;
void DeviceSetPadColorValue(const uint8_t mpcId, const uint8_t padNumber, const uint32_t rgbColorValue);