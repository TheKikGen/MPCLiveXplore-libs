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
// Router
#define ROUTER_SEQ_NAME "TKGL Midi"

#define ROUTER_CTRL_PORT_NAME "Ctrl"

#define MIDI_DECODER_SIZE 256

#define CTRL_MPC_ALL_PRIVATE "(MPC|Akai Pro Force).*Private$"

#define PRODUCT_CODE_PATH "/sys/firmware/devicetree/base/inmusic,product-code"

// Send and destination Ids for midi messages
enum FromToMidiIds  {  MPC_PRIVATE, MPC_PUBLIC,CTRL_MPC, MPC_EXTCTRL, CTRL_EXT };

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
  int portPriv; // Raw
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

// Declare in the same order that enums above
const static DeviceInfo_t DeviceInfoBloc[] = {
  { .productCode = "ACV5",  .productCompatible = "acv5",  .hasBattery = false, .sysexId = 0x3a,  .productString = "MPC X",       .productStringShort = "X",     .qlinkKnobsCount = 16, .sysexIdReply = {0x3A,0x00,0x19,0x00,0x01,0x01,0x01} },
  { .productCode = "ACV8",  .productCompatible = "acv8",  .hasBattery = true,  .sysexId = 0x3b,  .productString = "MPC Live",    .productStringShort = "LIVE",  .qlinkKnobsCount = 4,  .sysexIdReply = {0x3B,0x00,0x19,0x00,0x01,0x01,0x01} },
  { .productCode = "ADA2",  .productCompatible = "ada2",  .hasBattery = false, .sysexId = 0x40,  .productString = "Force",       .productStringShort = "FORCE", .qlinkKnobsCount = 8,  .sysexIdReply = {0x40,0x00,0x19,0x00,0x00,0x04,0x03} },
  { .productCode = "ACVA",  .productCompatible = "acva",  .hasBattery = false, .sysexId = 0x46,  .productString = "MPC One",     .productStringShort = "ONE",   .qlinkKnobsCount = 4,  .sysexIdReply = {0x46,0x00,0x19,0x00,0x01,0x01,0x01} },
  { .productCode = "ACVB",  .productCompatible = "acvb",  .hasBattery = true,  .sysexId = 0x47,  .productString = "MPC Live 2",  .productStringShort = "LIVE2", .qlinkKnobsCount = 4,  .sysexIdReply = {0x47,0x00,0x19,0x00,0x01,0x01,0x01} },
  { .productCode = "ACVB",  .productCompatible = "acvm",  .hasBattery = false, .sysexId = 0x4b,  .productString = "MPC Keys 61", .productStringShort = "KEY61", .qlinkKnobsCount = 4,  .sysexIdReply = {0x4B,0x00,0x19,0x00,0x01,0x01,0x01} },
  { .productCode = "ACV5S", .productCompatible = "acv5s", .hasBattery = false, .sysexId = 0x52,  .productString = "MPC XL",      .productStringShort = "XL",    .qlinkKnobsCount = 4,  .sysexIdReply = {0x52,0x00,0x19,0x00,0x01,0x01,0x01} },
  { .productCode = "ACVA2", .productCompatible = "acva2", .hasBattery = false, .sysexId = 0x56,  .productString = "MPC ONE 2",   .productStringShort = "ONE2",  .qlinkKnobsCount = 4,  .sysexIdReply = {0x56,0x00,0x19,0x00,0x01,0x01,0x01} },

};

// Function prototypes ---------------------------------------------------------

void ShowBufferHexDump(const uint8_t* data, ssize_t sz, uint8_t nl);
int match(const char *string, const char *pattern);
void dump_event(const snd_seq_event_t *ev);
int SeqSendRawMidi(snd_seq_t *seqHandle, uint8_t port,  const uint8_t *buffer, size_t size ) ;
