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
enum FromPortsIds  {  FROM_MPC_PRIVATE, FROM_MPC_PUBLIC,FROM_CTRL_MPC, FROM_MPC_EXTCTRL, FROM_CTRL_EXT };
enum ToPortsIds    {  TO_CTRL_MPC_PRIVATE, TO_CTRL_MPC_PUBLIC, TO_MPC_PRIVATE, TO_MPC_EXTCTRL, TO_CTRL_EXT };


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
