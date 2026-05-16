/* ------------------------------------------------------------------------------

S220718-R290823 - HW ref: A161125

IOS - I/O Subsystem for the  Z80-MBC2 (Multi Boot Computer - Z80 128kB RAM @ 4/8Mhz @ Fosc = 16MHz)


Notes:

1:  This SW is ONLY for the Atmega32A used as EEPROM and I/O subsystem (16MHz external oscillator) for 
    the Z80 CPU.
    
2:  Tested on Atmega32A/Atmega1284P @ Arduino IDE 1.8.19 and MightyCore v.2.2.2

3:  Embedded FW: S200718 iLoad (Intel-Hex loader)

4:  To run the stand-alone Basic and Forth interpreters the SD optional module must be installed with 
    the required binary files on a microSD (FAT16 or FAT32 formatted). Without the SD module you can 
    only run the iLoad loader.

5:  Utilities:   S111216 TASM conversion utility


---------------------------------------------------------------------------------

Credits:

SD library from: https://github.com/greiman/PetitFS (based on 
PetitFS: http://elm-chan.org/fsw/ff/00index_p.html)

PetitFS licence:
/-----------------------------------------------------------------------------/
/  Petit FatFs - FAT file system module  R0.03                  (C)ChaN, 2014
/-----------------------------------------------------------------------------/
/ Petit FatFs module is a generic FAT file system module for small embedded
/ systems. This is a free software that opened for education, research and
/ commercial developments under license policy of following trems.
/
/  Copyright (C) 2014, ChaN, all right reserved.
/
/ * The Petit FatFs module is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/
/-----------------------------------------------------------------------------/

Credits:

Thanks to Christian Welzel (http://www.welzel-online.ch) for some suggestions 
on Fuzix IRQ handling.

---------------------------------------------------------------------------------


CHANGELOG:


S220718           First revision.
S220718-R010918   Added "Disk Set" feature to manage multiple OS on SD (multi-booting).
                  Added support for QP/M 2.71 (with file names timestamping).
                  Added support for Atmega32A @ 20MHz (overclocked) to show the Z80 clock speed 
                   accordingly (Note that 20MHz is out of Atmega32A specifications!).
S220718-R190918   Added support for CP/M 3.
                  Fixed a bug in the manual RTC setting.
S220718-R260119   Changed the default serial speed to 115200 bps.
                  Added support for xmodem protocol (extended serial Rx buffer check and  
                   two new flags into the SYSFLAG Opcode for full 8 bit serial I/O control.
                  Added support for uTerm (A071218-R250119 and following revisions unless stated otherwise) 
                   reset at boot time.
S220718-R280819   Added a new Disk Set for the UCSD Pascal implementation (porting by Michel Bernard).
S220718-R240620   Added support for Collapse OS (https://collapseos.org/).
S220718-R290823   Added Fuzix OS support (www.fuzix.org):
                   changed Serial Rx interrupt generation;
                   added Systick interrupt.
                   added SETIRQ Opcode to enable/disable IRQ generation (see important notes inside the comments);
                   added ATXBUFF Opcode to check the available space of serial Tx buffer;
                   added SYSIRQ Opcode to check the triggered IRQ;
                   added SETTICK Opcode to set/change the Systick time;
                  Changed the behavior of the selection 3 of the boot menu and others minor changes.
                  Added support for the SPP Adapter (A240721-R270921 and following revisions unless stated 
                   otherwise) for parallel printers with 3 new Opcodes (SETSPP, WRSPP and GETSPP);
                   please note that now when the GPIO is set to operate as an SPP port all 
                   the GPIO write Opcodes (GPIOA Write, GPIOB Write, IODIRA Write, IODIRB Write, 
                   GPPUA Write, GPPUB Write) are ignored/disabled.
                  Added serial port speed selection inside the boot menu.
                  Added serial port Baud Recovery procedure. It can be triggered only if the serial port was
                   set at a speed not equal then the default (115200), pressing the RESET + USER keys and 
                   releasing the RESET key while holding down the USER key for about 4s until both USER and 
                   IOS leds blink quickly. At the next reboot the serial speed will be set at the default 
                   value (115200).
                  Added support to run on an Atmega1284/Atmega1284P MCU (leaving more space for customiztions).
                  Added SETOPT Opcode to enable/disable the message "CP/M WARM BOOT" (if CP/M CBIOS support 
                   this switch).
                  Now if the GPE expansion is found GPA0 and GPA2 are set with pullup enabled. This way if
                   the SPP adapter is used and a printer is connected, selected online and powered on before 
                   the Z80-MBC2, possible "strange" printer behaviors are avoided. This makes the STROBE and 
                   INIT lines of the parallel port not active after a power on/reset.

*** Changes for Z80_MBC2-V ***
*
* Original project's name: 'S220718-R290823_IOS-Z80-MBC2'
*
S071225-R071225   MCU_RTS_  renamed to SEL_WDOG ('SELECT' button input / hardware watchdog control)
                  MCU_CTS_  renamed to BUSACK (connected to processor's BUSACK signal)
                  BANK1     renamed to ETH_CS (ethernet SPI chip select)
                  BANK0     renamed to BANK_LATCH (single signal for 4-bit latch, 512KB ram, 16 banks)
                  RAM_CE2   renamed to RAM_OE_DIS (ram output disable, active high)
                  WAIT_RES_ renamed to WAIT_RES (active high instead of active low)
                  Opcode 0x0D handler (write bank) modified to use the new 16-banks latch from Z80 databus
                  Inverted the polarity of WAIT_RES, now active high
                  'loadByteToRAM() and 'loadHL()' modified to use RAM_OE_DIS instead of RAM_CE2
                   (inverted logic, can be left active as does not interfere with write cycles)

S071225-R131225   Added flag 'irqPending' (INT postpone)
                  Added 'currentBank' that contains the number of currently selected bank
                  Added 'irqSafeBank' writable by Z80 to specify the bank which is safe for INT
                  Interrupts are issued only if current bank is safe and a BANK SELECT ('ioOpcode') is not in progress
                  Added ethernet test (web server at 192.168.0.176 when main loop is running)

S071225-R211225   Added RTC SQW enable command (blinking led)
                  Added 'WRDISPCTRL' and 'WRDISPDATA' write opcodes (OLED display)

S071225-R241225   Display library removed, OLED now controlled by bare metal routines, optimized to reduce blocking time.
                  10ms system tick based on TIMER1 used for SEL_WDOG drive (watchdog test) and Z80 interrupt tick,
                   which is now selectable in 10ms increments instead of 1.

S071225-R251225   Ethernet library removed, implemented a bare metal telnet server that operates like the serial terminal.
                  Both interfaces are working simultaneously: what is typed on one, is also visible on the other.
                  TODO: atmega Serial.print / get char is still on serial interface only.

S071225-R261225   Completed telnet handling, which now works at any level through menu and Z80 applications.
                  Added option to set the power-on default RAM bank.
                   This allows program such as "1: Basic", who allows Warm Start, to run in a dedicated bank. Data (basic listing)
                   is retained due to the battery backup.
                  Added option to enable/disable the boot of previous menu selection.

S071225-R271225   Text to telnet client is now always through a TX FIFO.
                   This allows a client to receive all the menu text and selections that were made before connecting.
                   Hence, the variable banner is no longer necessary and has been removed.
                  Corrected a problem introduced with default bank selection: CP/M and Fuzix require BANK0 as initial bank:
                   they start loading without setting it.
                  When the option is a S.O., bank is forced to zero.
                  Solved a bug that prevented 'UCSD Pascal' from receiving keystrokes from telnet: the availability of characters
                   should (also) be notified through the bitmap returned by 'SYSFLAGS' command, which is used by the program
                   to determine when to poll for characters.

S071225-R281225   Z80 Clock selectable between 2MHz - 8MHz (@16MHz) for testing purposes.

S071225-R291225   Z80 INT not asserted (postponed) not only if a 0x0D opcode is in progress (SETBANK), but if ANY opcode is in progress.
                   This prevents a poorly written Z80 firmware from being interrupted during I/O OP, truncating an ongoing sequence.
                  Corrected a bug that left ioOpcode = 0x84 (datetime) unless the host popped an additional byte (clear ioOpcode on the byte beyond the last).
                  Temporarily reduced MAX_RAM_BANK to 2.
                  Temporarily set DEBUG to 2.
                  Many debug messages added.

S071225-R301225   Restored DEBUG = 1 and MAX_BANK = 15.
                  SD speed restored to 8MHz.
                  For fuzix, 'Z80IntSysTick' is now set to 0 by default: it turned out to be the reason for random kernel panic.
                  Some fuzix functions require the systick interrupt (i.e. che fscheck to finalize the sd write). In this case, please use the '[pwd]I'
                   telnet special command to temporarily enable tick INT just after fir final reboot message, then wait for leds to stop blinking.

S071225-R311225   Network parameters now stored in internal EEprom.
                  Network settings configuration menu.
                  The 'telnet_handler()' is now called on a 10ms basis or immediately (loop freerun) when more that a certain amount of characters
                   are waiting to be transmitted to a connected client. This saves time and reduces SPI overhead.
                  The display is turned off when a menu' selection is made.
                  The main menu' is displayed again after changing any option.
                  Other minor changes.

S071225-R010126   GPIO / I/O cycle optimization: direct register read/write and BUSACK signal from Z80 allows quite a better performance in timings.

S071225-R020126   Fixed a bug in the set baudrate function.
                  New telnet RAW mode: allows xmodem transfers.
                  Two new opcodes has heen created to set/get 'telnet_flags' register to enable/disable features; by now, olny bit 0 is in use
                   (1 = RAW mode enabled).
                  When RAW mode is enabled, special commands and any other character handling is disabled to allow raw binary data flow.
                  New telnet special command [pwd]R that completely restarts the board (atmega soft reset).

S071225-R030126   New 'GETBANK' opcode that returns the number of the currently active ram bank, very useful for setting the ISR-safe bank.
                  Telnet RAW mode is automatically disabled when a client disconnects.
                   This allows special commands to work (they can't be processed while in RAW mode).
                  If something bad happens between the telnet server and client, W5500 may start receiving many (nearly infinite) character,
                   preventing the RX LOOP from exiting and stalling the Z80 in WAIT at the first I/O attempt (because the FF doesn't get serviced).
                   The RX loop has now a "max-byte-per-iteration" counter and also exits immediately if the software RX FIFO is full (because Z80 is stalled).
                   Sooner or later, the RX flow will terminate.
                  WAIT poll before asserting INT has been commented out. INT can be asserted during an I/O operation and will be served upon completion
                   (original behavior).

S071225-R040125   New #defines 'NO_INT_ON_ANY_OPCODE' and 'POLL_WAIT_BEFORE_INT' to control the way INT is asserted. See comments in the INT assertion section.

S071225-R090125   Bugfix in telnet 'NORMALIZATION RULES' that prevents the W5500 RX buffer from being read ahead of received bytes.
                  Value of 'SERIAL_RX_BUFFER_SIZE' now printed.

S071225-R100126   New 'sh1106_contrast()' function to set the display brightness.
                  The display is now turned off after about 1 minute even if the system is in the main menu.
                  The display is dimmed after 25 seconds of inactivity.

S071225-R110126   RX serial buffer size increased to 2K (1284.menu.LTO.Os.compiler.cpp.extra_flags=-DSERIAL_RX_BUFFER_SIZE=2048)
                  'DEBUG' level set to zero.

S071225-R130126   Changed default MAC address.
                  Security key 'yes' needed to confirm Network Config changes.

S071225-R310126   The SETTELNETFLAGS opcode is now a two-bytes command. The first byte is the SET mask and the second byte is the RESET mask.
                   This allows set/reset of a single bit without having to read previous state of the 'telnet_flags' register.
                  Support for new 'TELNET' device in CP/M 3.0 bios:
                   Telnet I/O is no longer bridged (copied) by default to IOS serial I/O.
                   Previous bridged mode is now enabled when new 'TFLAG_CON_BRIDGE' bit in 'telnet_flags' is SET (default '0', not enabled).
                   This bit can be (also) toggled by new telnet special command "[pwd]B", which allows gaining remote control of device.
                   Before this step, a telnet client does not transfer any byte to IOS nor Z80 serial I/O, but only through the 'TELNET' dedicated opcodes.
                  Three new opcodes defined:
                   0x44 TELNET TX: works as 'SERIAL TX' but sends a byte over telnet only;
                   0xC3 TELNET RX: works as 'SERIAL RX' but returns telnet received bytes only (unlike SERIAL RX, requires an opcode write for each character);
                   0xC2 GET TELNET STATUS, where d0 is 'available received bytes' flag and d1 is 'client is connected' flag.
                   These opcodes are used by the new 'TELNET' device defined in 'CHARIO.MAC' bios source of CP/M 3.0.

S071225-R030226   Two telnet socket now listening on different ports.
                  The SOCKET0 is still binded to the serial console when the 'TFLAG_CON_BRIDGE' bit is SET.
                  Telnet opcodes modified as follows:
                   0xC2 GETTELNETSTAT03  now returns pairs of "client connected"/"data available" bits for (up to) four sockets
                   0xC3 TELNET RX S0     rx bytes opcode for SOCKET 0, can be used only when the 'TFLAG_CON_BRIDGE' gridging mode is NOT active
                   0xC4 TELNET RX S1     rx bytes for SOCKET 1
                   0x43 SELTELNETSOCKET  allows setting the socket number for subsequent flags/status read/write operations
                   0x44 SETTELNETFLAGS   sets telnet flag byte for the currently selected socket
                   0x45  TELNET TX S0    tx bytes opcode for SOCKET 0. Unlike the RX function, this works regardless of the state of 'TFLAG_CON_BRIDGE' flag.
                   0x46  TELNET TX S1    tx bytes opcode for SOCKET 1.

S071225-R070226   New 'telnet_flags' bit 'TFLAG_DISCONNECT' that forces a client disconnection on selected socket.
                  Fixed a bug that prevented DISCONNECT cleanup upon call to 'w5500_disconnect()' because the function was setting 'last_state' to 'SOCK_CLOSED'.

S071225-R100226   New opcodes 0x49 and 0x4A for OR - AND bitmap write on GPE PORTA and PORTB respectively
                  Telnet port are now 2300 + socket number (i.e. 2300 for socket 0, 2301 for socket 1, etc)

S071225-R140226   New 'telnet_flags' bit 'TFLAG_PURGETXONCONN' that purges the telnet TX FIFO upon new client connection

S071225-R160226   When the telnet flag 'TFLAG_PURGETXONCONN' is reset (by SETTELNETFLAGS) and the socket is not connected, the TX fifo gets purged one last time.
                  The telnet flag 'TFLAG_CON_BRIDGE' is now meaningful also for socket other than 0: traffic gets forwarded from socket 'x' to serial and socket 0 (but not vice-versa)

S071225-R280226   Reloading the menu also displays the hardware status and RTC set
                  Removed prompt for RTC setting in case of oscillator failure (the machine may look bricked while waiting for Y/N if accessed by telnet)

S071225-R070326   New 0x4B opcode for Z80 / menu' watchdog control

S071225-R100326   The telnet handler now detects if client is telnet or not, setting the 'is_telnet' flag. If so, unescapes IAC 0xFF even in RAW mode. This allows syncTerm XMODEM transfers.
                  The negotiate_character_mode on new telnet connection is only invoked if the new telnet_flag 'TFLAG_FORCE_NEGO_C_M' is set.

S071225-R110326   If client is telnet, also escapes 0xFF (IAC) bytes when sending (adds one more 0xFF byte)

S071225-R120326   The negotiate_character_mode() function is now automatically called when a telnet client connects (see above the 'is_telnet' flag).
                  Flag 'TFLAG_FORCE_NEGO_C_M' can still be used to force the call.
                  The IAC negotiation now also forces binary mode on client (preventing teraTerm from adding 0x00 bytes after 0x0D).

S071225-R130326   A small IAC packet (IAC DO SUPPRESS-GO-AHEAD) is sent upon new connection to see if an otherwise silent telnet client responds and triggers the above detection.

S071225-R250426   Oled display turned on by USER / SELECT keypress; socket0 password now defined as char (that may be set in EEprom)

S071225-R270426   Optimized Wiznet5500 driver (16 bit rd/wr, 4KB buffers, pre-compiled 'block' addresses, 1 minute keepalive not working)

S071225-R280426   Further Wiznet5500 optimization, working keepalive, remote IP captured and readable by new 0xC7 opcode.

S071225-R010526   'SETTELNETFLAGS' (0x44) handler now always clears iac_state if RAW_MODE is set or reset

S071225-R060526   Socket0 password now editable and EE-saved

S071225-R130526   If a client connects through a proxy and a PROXY Protocol V1 string is received, the real client's IP is parsed and stored in place of proxy's IP.
                   For this to work, the proxy's IP (LAN) should be configured as the "Trusted Proxy" in network configuration.
                  Random cleanup.

S071225-R160526   Date / time of last Z80 watchdog reset is saved and displayed on OLED and console menu';
                  Console baud rate is also printed on OLED display, useful if you're unable to connect the serial port;
                  Fixed a bug that prevented AUTOEXEC from running if menu was bypassed;
                  Fixed a bug that prevented RTC from being read if menu was bypassed.


Tempi pre-modifiche BUSACK controllo pin @8MHz:

CP/M 3.0 --> prompt   8,5 sec
FUZIX    --> login    90 sec
FUZIX    --> shutdown 61 sec
FUZIX banner "Z80_MNC2-V" 9 sec al prompt; secondo giro ~4 sec
CICLO for n = 0 to 10000: next = 45 sec

Post-modifiche:

CP/M 3.0 --> prompt   6,1 sec
FUZIX    --> login    68 sec
FUZIX    --> shutdown 45 sec
FUZIX banner "Z80_MNC2-V" 6,5 sec al prompt; secondo giro ~3,5 sec
CICLO for n = 0 to 10000: next = 38,5 sec

Tempi @10MHz
CP/M 3.0 --> prompt   5,1 sec
FUZIX    --> login    56 sec
FUZIX    --> shutdown 36 sec
FUZIX banner "Z80_MNC2-V" 5,5 sec al prompt; secondo giro ~3 sec
CICLO for n = 0 to 10000: next = 36,5 sec


--------------------------------------------------------------------------------- */

#define   HW_REV        "A060126"
#define   IO_SUBS_BEGIN "S071225"
#define   IO_SUBS_END   "R160526"

// ------------------------------------------------------------------------------
//
// Hardware definitions - Base system
//
// ------------------------------------------------------------------------------

#define   DEBUG                   1                 // Debug off = 0, light = 1, on = 2, on with interrupt trace and opcodes trace = 3, on with SEL_WDOG as trigger = 4

#define   MAX_RAM_BANK            14                // 512KB (14 does not allow selecting the fixed bank, which is #15)
#define   DEFAULT_RAM_BANK        3                 // Bank selected if eeprom blank. This is a free bank (not used by CP/M nor RAMDISK)

#define   Z80_MIN_CLOCK           1000000UL         // Minimum Z80 clock frequency (Hz)

#define   NO_INT_ON_ANY_OPCODE    0                 // 1 = INT is not asserted if any opcode active; 0 = not asserted during SETBANK only
#define   POLL_WAIT_BEFORE_INT    0                 // 1 = INT is not asserted if a WAIT state (I/O) is occurring; 0 = INT asserted regardless of WAIT

#define   SOCKET0PWD_DEF          "s0password"      // default socket0 password for bridging activation and special commands

// ------------------------------------------------------------------------------
//
// Pins definitions
//
// ------------------------------------------------------------------------------

#define   D0                      24                // PA0 pin 40   Z80 data bus (D0-D7)
#define   D1                      25                // PA1 pin 39
#define   D2                      26                // PA2 pin 38
#define   D3                      27                // PA3 pin 37
#define   D4                      28                // PA4 pin 36
#define   D5                      29                // PA5 pin 35
#define   D6                      30                // PA6 pin 34
#define   D7                      31                // PA7 pin 33

#define   AD0                     18                // PC2 pin 24   Z80 A0
#define   AD0_PORTIN              PINC
#define   AD0_PIN                 2
#define   WR_                     19                // PC3 pin 25   Z80 WR (active low)
#define   WR_PORTIN               PINC
#define   WR_PIN                  3
#define   RD_                     20                // PC4 pin 26   Z80 RD (active low)
#define   RD_PORTIN               PINC
#define   RD_PIN                  4
#define   MREQ_                   21                // PC5 pin 27   Z80 MREQ (active low)
#define   RESET_                  22                // PC6 pin 28   Z80 RESET (active low)
#define   SEL_WDOG                23                // PC7 pin 29   'SELECT button / hardware watchdog trigger
#define   SEL_WDOG_PORT           PORTC
#define   SEL_WDOG_PORTIN         PINC
#define   SEL_WDOG_DDR            DDRC
#define   SEL_WDOG_PIN            7
#define   BUSACK_                 10                // PD2 pin 16   Z80 BUSACK
#define   BUSACK_PORTIN           PIND
#define   BUSACK_PIN              2
//#define ETH_CS_                 11                // PD3 pin 17   Definito in 'w5500.h'
#define   BANK_LATCH              12                // PD4 pin 18   Was RAM Memory bank address (Low), now global bank latch
#define   BANK_LATCH_PORT         PORTD
#define   BANK_LATCH_PIN          4
#define   INT_                    1                 // PB1 pin 2    Z80 INT (active low)
#define   INT_PORT                PORTB
#define   INT_PIN                 1
#define   RAM_OE_DIS              2                 // PB2 pin 3    RAM output (read) disable. Active High. Used only during boot
#define   WAIT_                   3                 // PB3 pin 4    Z80 WAIT (active low)
#define   WAIT_PORTIN             PINB
#define   WAIT_PIN                3
//#define SS_                     4                 // PB4 pin 5    SD SPI (active low) (definito in 'w5500.h')
#define   MOSI                    5                 // PB5 pin 6    SD SPI
#define   MISO                    6                 // PB6 pin 7    SD SPI
#define   SCK                     7                 // PB7 pin 8    SD SPI
#define   BUSREQ_                 14                // PD6 pin 20   Z80 BUSRQ (active low)
#define   BUSREQ_PORT             PORTD
#define   BUSREQ_PIN              6
#define   CLK                     15                // PD7 pin 21   Z80 CLK
#define   SCL_PC0                 16                // PC0 pin 22   IOEXP connector (I2C)
#define   SDA_PC1                 17                // PC1 pin 23   IOEXP connector (I2C)
#define   LED_IOS                 0                 // PB0 pin 1    Led LED_IOS is ON if HIGH
#define   WAIT_RES                0                 // PB0 pin 1    Reset the Wait FF (active high)
#define   WAIT_RES_PORT           PORTB
#define   WAIT_RES_PIN            0
#define   USER                    13                // PD5 pin 19   Led USER and key (led USER is ON if LOW)
#define   USER_PORT               PORTD
#define   USER_PORTIN             PIND
#define   USER_DDR                DDRD
#define   USER_PIN                5

// ------------------------------------------------------------------------------
//
// Hardware definitions for GPE Option (Optional GPIO Expander)
//
// ------------------------------------------------------------------------------

#define   GPIOEXP_ADDR            0x20              // I2C module address (see datasheet)
#define   IODIRA_REG              0x00              // MCP23017 internal register IODIRA  (see datasheet)
#define   IODIRB_REG              0x01              // MCP23017 internal register IODIRB  (see datasheet)
#define   GPPUA_REG               0x0C              // MCP23017 internal register GPPUA  (see datasheet)
#define   GPPUB_REG               0x0D              // MCP23017 internal register GPPUB  (see datasheet)
#define   GPIOA_REG               0x12              // MCP23017 internal register GPIOA  (see datasheet)
#define   GPIOB_REG               0x13              // MCP23017 internal register GPIOB  (see datasheet)

// ------------------------------------------------------------------------------
//
// Hardware definitions for RTC Module Option (see DS3231 datasheet)
//
// ------------------------------------------------------------------------------

#define   DS3231_RTC              0x68              // DS3231 I2C address
#define   DS3231_SECRG            0x00              // DS3231 Seconds Register
#define   DS3231_CONTRG           0x0E              // DS3231 Control Register
#define   DS3231_STATRG           0x0F              // DS3231 Status Register

// ------------------------------------------------------------------------------
//
// File names and starting addresses
//
// ------------------------------------------------------------------------------

#define   BASICFN                 "BASIC47.BIN"     // "ROM" Basic
#define   FORTHFN                 "FORTH13.BIN"     // "ROM" Forth
#define   CPMFN                   "CPM22.BIN"       // CP/M 2.2 loader
#define   QPMFN                   "QPMLDR.BIN"      // QP/M 2.71 loader
#define   CPM3FN                  "CPMLDR.COM"      // CP/M 3 CPMLDR.COM loader
#define   UCSDFN                  "UCSDLDR.BIN"     // UCSD Pascal loader
#define   COSFN                   "COS.BIN"         // Collapse OS loader
#define   FUZIXFN                 "FUZIX.BIN"       // Fuzix OS loader
#define   AUTOFN                  "AUTOBOOT.BIN"    // Autoboot.bin file
#define   Z80DISK                 "DSxNyy.DSK"      // Generic Z80 disk name (from DS0N00.DSK to DS9N99.DSK)
#define   DS_OSNAME               "DSxNAM.DAT"      // File with the OS name for Disk Set "x" (from DS0NAM.DAT to DS9NAM.DAT)
#define   BASSTRADDR              0x0000            // Starting address for the stand-alone Basic interptreter
#define   FORSTRADDR              0x0100            // Starting address for the stand-alone Forth interptreter
#define   CPM22CBASE              0xD200            // CBASE value for CP/M 2.2
#define   CPMSTRADDR              (CPM22CBASE - 32) // Starting address for CP/M 2.2
#define   QPMSTRADDR              0x80              // Starting address for the QP/M 2.71 loader
#define   CPM3STRADDR             0x100             // Starting address for the CP/M 3 loader
#define   UCSDSTRADDR             0x0000            // Starting address for the UCSD Pascal loader
#define   COSSTRADDR              0x0000            // Starting address for the Collapse OS loader
#define   FUZSTRADDR              0x0000            // Starting address for the Fuzix OS loader
#define   AUTSTRADDR              0x0000            // Starting address for the AUTOBOOT.BIN file

// ------------------------------------------------------------------------------
//
// Internal EEPROM addresses
//
// ------------------------------------------------------------------------------

#define   EE_Z80WATCHDOG_ADDR     0                 // Internal EEPROM address for Z80 watchdog counter's structure
#define   EEPROM_NETCFG_ADDR      10                // Internal EEPROM address for Network Configuration's structure
//
#define   EE_BOOTMODE_ADDR        100               // Internal EEPROM address for boot mode storage
#define   EE_AUTOEXECFLAG_ADDR    101               // Internal EEPROM address for AUTOEXEC flag storage
#define   EE_CLOCKDIVIDER_ADDR    102               // Internal EEPROM address for the Z80 clock speed divider
#define   EE_DISKSET_ADDR         103               // Internal EEPROM address for the current Disk Set [0..9]
#define   EE_SERBAUD_ADDR         104               // Internal EEPROM address for the current serial speed index
#define   EE_DEFBANK_ADDR         105               // Internal EEPROM address for default ram bank at power-on
#define   EE_REMEMBERLASTSEL_ADDR 106               // Internal EEPROM address for last menu selection flag

// ------------------------------------------------------------------------------
//
// Atmega clock speed check
//
// ------------------------------------------------------------------------------

#if F_CPU == 20000000
  #define OCR1A_VALUE             3124;             // 10 ms @ 20 MHz TIMER1 tick
#else
  #define OCR1A_VALUE             2499;             // 10 ms @ 16 MHz TIMER1 tick
#endif

//
// KEY DEFINES (NETWORK SETTINGS)
//
#define KEY_NONE                  0
#define KEY_UP                    1
#define KEY_DOWN                  2
#define KEY_LEFT                  3
#define KEY_RIGHT                 4
#define KEY_SHIFT_UP              5
#define KEY_SHIFT_DOWN            6
#define KEY_ENTER                 7
#define KEY_ESC                   8
#define KEY_BACKSPACE             9
#define KEY_DIGIT                 10
#define KEY_SAVE                  11

// ------------------------------------------------------------------------------
//
//  Libraries
//
// ------------------------------------------------------------------------------

#include <avr/pgmspace.h>                 // Needed for PROGMEM
#include "Wire.h"                         // Needed for I2C bus
#include <EEPROM.h>                       // Needed for internal EEPROM R/W
#include "PetitFS.h"                      // Light handler for FAT16 and FAT32 filesystems on SD
// ethernet
#include "w5500.h"
// display
#include "sh1106.h"
#include "font6x8.h"
#include "font9x13.h"


// ------------------------------------------------------------------------------
//
//  Constants
//
// ------------------------------------------------------------------------------

const byte    LD_HL            =  0x36;       // Z80 instruction: LD(HL), n
const byte    INC_HL           =  0x23;       // Z80 instruction: INC HL
const byte    LD_HLnn          =  0x21;       // Z80 instruction: LD HL, nn
const byte    JP_nn            =  0xC3;       // Z80 instruction: JP nn
const String  compTimeStr      = __TIME__;    // Compile timestamp string
const String  compDateStr      = __DATE__;    // Compile datestamp string
const byte    daysOfMonth[]    = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
const uint32_t baudIndex[]     = {600, 1200, 2400, 4800, 9600, 19200, 28800, 38400, 57600, 115200};
const byte    maxBaudIndex     = 10;          // Max number of serial speed values
const byte    maxDiskNum       = 99;          // Max number of virtual disks
const byte    maxDiskSet       = 6;           // Number of configured Disk Sets

// Z80 programs images into flash and related constants
const word  boot_A_StrAddr = 0xfd10;      // Payload A image starting address (flash)
const byte  boot_A_[] PROGMEM = {         // Payload A image (S200718 iLoad)
  0x31, 0x10, 0xFD, 0x21, 0x52, 0xFD, 0xCD, 0xC6, 0xFE, 0xCD, 0x3E, 0xFF, 0xCD, 0xF4, 0xFD, 0x3E, 
  0xFF, 0xBC, 0x20, 0x10, 0xBD, 0x20, 0x0D, 0x21, 0xD9, 0xFD, 0xCD, 0xC6, 0xFE, 0x21, 0x88, 0xFD, 
  0xCD, 0xC6, 0xFE, 0x76, 0xE5, 0x21, 0xD9, 0xFD, 0xCD, 0xC6, 0xFE, 0x21, 0x75, 0xFD, 0xCD, 0xC6, 
  0xFE, 0xE1, 0xCD, 0x4B, 0xFF, 0xCD, 0x3E, 0xFF, 0xCD, 0x3E, 0xFF, 0xDB, 0x01, 0xFE, 0xFF, 0x20, 
  0xFA, 0xE9, 0x69, 0x4C, 0x6F, 0x61, 0x64, 0x20, 0x2D, 0x20, 0x49, 0x6E, 0x74, 0x65, 0x6C, 0x2D, 
  0x48, 0x65, 0x78, 0x20, 0x4C, 0x6F, 0x61, 0x64, 0x65, 0x72, 0x20, 0x2D, 0x20, 0x53, 0x32, 0x30, 
  0x30, 0x37, 0x31, 0x38, 0x00, 0x53, 0x74, 0x61, 0x72, 0x74, 0x69, 0x6E, 0x67, 0x20, 0x41, 0x64, 
  0x64, 0x72, 0x65, 0x73, 0x73, 0x3A, 0x20, 0x00, 0x4C, 0x6F, 0x61, 0x64, 0x20, 0x65, 0x72, 0x72, 
  0x6F, 0x72, 0x20, 0x2D, 0x20, 0x53, 0x79, 0x73, 0x74, 0x65, 0x6D, 0x20, 0x68, 0x61, 0x6C, 0x74, 
  0x65, 0x64, 0x00, 0x57, 0x61, 0x69, 0x74, 0x69, 0x6E, 0x67, 0x20, 0x69, 0x6E, 0x70, 0x75, 0x74, 
  0x20, 0x73, 0x74, 0x72, 0x65, 0x61, 0x6D, 0x2E, 0x2E, 0x2E, 0x00, 0x53, 0x79, 0x6E, 0x74, 0x61, 
  0x78, 0x20, 0x65, 0x72, 0x72, 0x6F, 0x72, 0x21, 0x00, 0x43, 0x68, 0x65, 0x63, 0x6B, 0x73, 0x75, 
  0x6D, 0x20, 0x65, 0x72, 0x72, 0x6F, 0x72, 0x21, 0x00, 0x69, 0x4C, 0x6F, 0x61, 0x64, 0x3A, 0x20, 
  0x00, 0x41, 0x64, 0x64, 0x72, 0x65, 0x73, 0x73, 0x20, 0x76, 0x69, 0x6F, 0x6C, 0x61, 0x74, 0x69, 
  0x6F, 0x6E, 0x21, 0x00, 0xF5, 0xD5, 0xC5, 0x01, 0xFF, 0xFF, 0x21, 0xA3, 0xFD, 0xCD, 0xC6, 0xFE, 
  0xCD, 0x3E, 0xFF, 0xCD, 0x72, 0xFF, 0xFE, 0x0D, 0x28, 0xF9, 0xFE, 0x0A, 0x28, 0xF5, 0xFE, 0x20, 
  0x28, 0xF1, 0xCD, 0x1A, 0xFF, 0xCD, 0x69, 0xFF, 0xFE, 0x3A, 0xC2, 0xA3, 0xFE, 0xCD, 0xE1, 0xFE, 
  0x57, 0x1E, 0x00, 0xCD, 0xBE, 0xFE, 0xCD, 0xD6, 0xFE, 0x3E, 0xFF, 0xB8, 0x20, 0x05, 0xB9, 0x20, 
  0x02, 0x44, 0x4D, 0x7C, 0xCD, 0xBE, 0xFE, 0x7D, 0xCD, 0xBE, 0xFE, 0xCD, 0xE1, 0xFE, 0xCD, 0xBE, 
  0xFE, 0xFE, 0x01, 0x20, 0x1E, 0xCD, 0xE1, 0xFE, 0xCD, 0xBE, 0xFE, 0x7B, 0xA7, 0x28, 0x66, 0xCD, 
  0x3E, 0xFF, 0x21, 0xD9, 0xFD, 0xCD, 0xC6, 0xFE, 0x21, 0xC9, 0xFD, 0xCD, 0xC6, 0xFE, 0x01, 0xFF, 
  0xFF, 0x18, 0x52, 0x7A, 0xA7, 0x28, 0x2C, 0xCD, 0xE1, 0xFE, 0xCD, 0xBE, 0xFE, 0xE5, 0xC5, 0xA7, 
  0x01, 0xF0, 0xFC, 0xED, 0x42, 0xC1, 0xE1, 0xDA, 0x8E, 0xFE, 0xCD, 0x3E, 0xFF, 0x21, 0xD9, 0xFD, 
  0xCD, 0xC6, 0xFE, 0x21, 0xE1, 0xFD, 0xCD, 0xC6, 0xFE, 0x01, 0xFF, 0xFF, 0x18, 0x27, 0x77, 0x23, 
  0x15, 0x18, 0xD0, 0xCD, 0xE1, 0xFE, 0xCD, 0xBE, 0xFE, 0x7B, 0xA7, 0x20, 0xB2, 0xCD, 0x3E, 0xFF, 
  0xC3, 0x03, 0xFE, 0xCD, 0x3E, 0xFF, 0x21, 0xD9, 0xFD, 0xCD, 0xC6, 0xFE, 0x21, 0xBB, 0xFD, 0xCD, 
  0xC6, 0xFE, 0x01, 0xFF, 0xFF, 0xCD, 0x3E, 0xFF, 0x60, 0x69, 0xC1, 0xD1, 0xF1, 0xC9, 0xC5, 0x4F, 
  0x7B, 0x91, 0x5F, 0x79, 0xC1, 0xC9, 0xF5, 0xE5, 0x7E, 0xFE, 0x00, 0x28, 0x06, 0xCD, 0x69, 0xFF, 
  0x23, 0x18, 0xF5, 0xE1, 0xF1, 0xC9, 0xF5, 0xCD, 0xE1, 0xFE, 0x67, 0xCD, 0xE1, 0xFE, 0x6F, 0xF1, 
  0xC9, 0xC5, 0xCD, 0xF4, 0xFE, 0xCB, 0x07, 0xCB, 0x07, 0xCB, 0x07, 0xCB, 0x07, 0x47, 0xCD, 0xF4, 
  0xFE, 0xB0, 0xC1, 0xC9, 0xCD, 0x72, 0xFF, 0xCD, 0x1A, 0xFF, 0xCD, 0x06, 0xFF, 0x30, 0xF5, 0xCD, 
  0x23, 0xFF, 0xCD, 0x2E, 0xFF, 0xC9, 0xFE, 0x47, 0xD0, 0xFE, 0x30, 0x30, 0x02, 0x3F, 0xC9, 0xFE, 
  0x3A, 0xD8, 0xFE, 0x41, 0x30, 0x02, 0x3F, 0xC9, 0x37, 0xC9, 0xFE, 0x61, 0xD8, 0xFE, 0x7B, 0xD0, 
  0xE6, 0x5F, 0xC9, 0xFE, 0x3A, 0x38, 0x02, 0xD6, 0x07, 0xD6, 0x30, 0xE6, 0x0F, 0xC9, 0xF5, 0xE6, 
  0x0F, 0xC6, 0x30, 0xFE, 0x3A, 0x38, 0x02, 0xC6, 0x07, 0xCD, 0x69, 0xFF, 0xF1, 0xC9, 0xF5, 0x3E, 
  0x0D, 0xCD, 0x69, 0xFF, 0x3E, 0x0A, 0xCD, 0x69, 0xFF, 0xF1, 0xC9, 0xE5, 0xF5, 0x7C, 0xCD, 0x58, 
  0xFF, 0x7D, 0xCD, 0x58, 0xFF, 0xF1, 0xE1, 0xC9, 0xF5, 0xC5, 0x47, 0x0F, 0x0F, 0x0F, 0x0F, 0xCD, 
  0x2E, 0xFF, 0x78, 0xCD, 0x2E, 0xFF, 0xC1, 0xF1, 0xC9, 0xF5, 0x3E, 0x01, 0xD3, 0x01, 0xF1, 0xD3, 
  0x00, 0xC9, 0xDB, 0x01, 0xFE, 0xFF, 0xCA, 0x72, 0xFF, 0xC9
  };

const byte * const flahBootTable[1] PROGMEM = {boot_A_}; // Payload pointers table (flash)


// ------------------------------------------------------------------------------
//  STRUCTURES
// ------------------------------------------------------------------------------

struct RTC_St {
    byte  foundRTC;                       // Set to 1 if RTC is found, 0 otherwise
    byte  OscStopFlag;                    // Set to 1 if oscillator stopped (unreliable time/date)
    byte  seconds, minutes, hours;
    byte  day, month, year;
    byte tempC;                           // Temperature (Celsius) encoded in two’s complement integer format
};

struct Z80_WDOG_St {                      // max size = 10 bytes (see 'EE_Z80WATCHDOG_ADDR')
    uint32_t count;                       // wdog event occurrences
    byte  seconds, minutes, hours;        // time of last occurrence
    byte  day, month, year;
};


// ------------------------------------------------------------------------------
//
//  Global variables
//
// ------------------------------------------------------------------------------

// General purpose variables and types
enum          baudRecCheck  {BLK, CHECK}; // Used to set on/off the baud recovery option in the boot menu
byte          ioAddress;                  // Virtual I/O address. Only two possible addresses are valid (0x00 and 0x01)
byte          ioData;                     // Data byte used for the I/O operation
byte          ioOpcode;                   // I/O operation code or Opcode (0xFF means "No Operation")
word          ioByteCnt;                  // Exchanged bytes counter during an I/O operation
byte          moduleGPIO     = 0;         // Set to 1 if the module is found, 0 otherwise
byte          SPPmode;                    // Set to 1 if the GPIO port is used as a standard SPP (SPP Adapter)
byte          SPPautofd;                  // Store the status of the AUTOFD Control Line (active Low) of the SPP
byte          bootMode;                   // Set the program to boot (from flash or SD)
byte *        BootImage;                  // Pointer to selected flash payload array (image) to boot
word          BootImageSize;              // Size of the selected flash payload array (image) to boot
word          BootStrAddr;                // Starting address of the selected program to boot (from flash or SD)
byte          Z80IntRx;                   // Z80 serial Rx INT_ enable flag (0 = disabled, 1 = enabled)
byte          Z80IntSysTick;              // Z80 Systick INT_ enable flag (0 = disabled, 1 = enabled)
char          inChar;                     // Input char from serial
byte          iCount;                     // Temporary variable (counter)
byte          clockDivider;               // Z80 clock divider
byte          LastRxIsEmpty;              // "Last Rx char was empty" flag. Is set when a serial Rx operation was done
                                          //  when the Rx buffer was empty
byte          irqStatus;                  // Store the interrupr status byte (every bit is the status of a different 
                                          //  interrupt. See the SYSIRQ Opcode)
byte          irqPending;                 // '1' when _INT_ should be pulled low (to generate an INT to Z80)
byte          currentBank;                // keeps track of current selected memory bank
byte          irqSafeBank;                // bank safe for issuing INT
byte          sysTickTime;                // Period in milliseconds of the Z80 Systick interrupt (if enabled)
byte          sysTickCnt;                 // Z80 interrupt systick counter
byte          tick100ms;                  // general purpose 100ms tick prescaler
byte          tick1s;                     // general purpose 1s tick prescaler
byte          RxDoneFlag;                 // This flag is set (= 1) soon after a Serial Rx operation (used for Rx interrupt control)
byte          cpmWarmBootFlg;             // This flag enable/disable (1/0) the message "CP/M WARM BOOT" if 
                                          //  the CP/M CBIOS supports this switch (see the SETOPT Opcode)
byte          telnetSpecialCmds;          // special commands from telnel client
byte          inTheMenu;                  // '1' when menu is active (Z80 idle)
byte          buttonsState = 0;           // d0 = USER KEY; d1 = SELECT KEY

byte          Z80WatchDOG_time;           // Z80 watchdog expire time (*10s)
byte          Z80Watchdog_tmp;
byte          Z80Watchdog_cnt;
byte          Z80Watchdog_prescaler;
bool          Z80watchdog_ioOper;
bool          Z80watchdog_NOioOper;

Z80_WDOG_St   Z80wdog_counters;

// DS3231 RTC variables
RTC_St        rtcData;

// SD disk and CP/M support variables
FATFS         filesysSD;                  // Filesystem object (PetitFS library)
byte          bufferSD[32];               // I/O buffer for SD disk operations (store a "segment" of a SD sector).
                                          //  Each SD sector (512 bytes) is divided into 16 segments (32 bytes each)
const char *  fileNameSD;                 // Pointer to the string with the currently used file name
byte          autobootFlag;               // Set to 1 if "autoboot.bin" must be executed at boot, 0 otherwise
byte          autoexecFlag;               // Set to 1 if AUTOEXEC must be executed at CP/M cold boot, 0 otherwise
byte          errCodeSD;                  // Temporary variable to store error codes from the PetitFS
byte          numReadBytes;               // Number of read bytes after a readSD() call

// Disk emulation on SD
char          diskName[11]    = Z80DISK;  // String used for virtual disk file name
char          OsName[11]      = DS_OSNAME;// String used for file holding the OS name
word          trackSel;                   // Store the current track number [0..511]
byte          sectSel;                    // Store the current sector number [0..31]
byte          diskErr         = 19;       // SELDISK, SELSECT, SELTRACK, WRITESECT, READSECT or SDMOUNT resulting 
                                          //  error code
byte          numWriBytes;                // Number of written bytes after a writeSD() call
byte          diskSet;                    // Current "Disk Set"

// Ethernet
extern NetConfig netcfg;
extern TelnetSession telnet_sessions[];
extern bool   eth_ok;
byte          ethTick;                    // ethernet polling tick
byte          telnetSocketSel = 0;        // selected telnet socket for sending / polling bytes

// Display
typedef enum {
    DISPOP_IDLE = 0,
    DISPOP_CLEAR,
    DISPOP_SETPOS,
    DISPOP_WRCHAR,
    DISPOP_SETCONTRAST
} dispOp_t;

bool          disp_ok         = false;
byte          disp_X          = 0;            // X write coordinate
byte          disp_Y          = 0;            // Y write coordinate
dispOp_t      dispOp          = DISPOP_IDLE;  // display operation pending
byte          dispData        = 0;            // character to display / command parameter
byte          bufferDISP[10];

// GPE
byte          gpioPortA_M = 0;
byte          gpioPortB_M = 0;


// ------------------------------------------------------------------------------
//  FUNCTION PROTOTYPES
// ------------------------------------------------------------------------------

void consolePrint(const char *, ...);
void SetRAMBank(uint8_t);
void LoadNetworkConfig(void);
void printMsg1(void);
byte WaitAndBlink(baudRecCheck);
byte decToBcd(byte);
byte bcdToDec(byte);
void readRTC(RTC_St *);
void writeRTC(RTC_St *);
void RTCCheck(RTC_St *);
void printDateTime(RTC_St *);
void print2digit(byte);
byte isLeapYear(byte);
void ChangeRTC(void);
void ChangeDefaultBANK(void);
void ChangeZ80Clock(void);
bool ChangeNetworkConfig(void);
void FormatMAC(char *, uint8_t);
uint8_t ReadKey(char *);
void FormatNumber(char *, uint16_t);
void FormatIPv4(char *, uint8_t);
void PrintRow(uint8_t, uint8_t, const char *, const char *);
void RedrawRow(uint8_t, uint8_t, NetConfig *);
void DrawNetworkTable(uint8_t, NetConfig);
void EditFieldInline(uint8_t *, uint8_t count, uint8_t);
void EditMacInline(uint8_t *, uint8_t);
void MoveCursorToField(uint8_t, uint8_t, uint8_t);
uint16_t ReadNumberFromConsole(void);
bool ReadStringFromConsole(char*, uint8_t);
void pulseClock(byte);
void loadByteToRAM(byte);
void loadHL(word);
void singlePulsesResetZ80(void);
byte mountSD(FATFS*);
byte openSD(const char*);
byte readSD(void*, byte*);
byte writeSD(void*, byte*);
byte seekSD(word);
void printErrSD(byte, byte, const char*);
void waitKey(void);
void printOsName(byte);
void FlushSerials(void);
void Z80_async_reset(void);

// ------------------------------------------------------------------------------

void setup() 
{
  // ----------------------------------------
  // INITIALIZATION
  // ----------------------------------------

  // Initialize RESET_ and WAIT_RES
  pinMode(RESET_, OUTPUT);                                                    // Configure RESET_ and set it ACTIVE
  digitalWrite(RESET_, LOW);
  pinMode(WAIT_RES, OUTPUT);                                                  // Configure WAIT_RES and set it ACTIVE to reset the WAIT FF
  digitalWrite(WAIT_RES, HIGH);

  DDRA = 0x00;                                                                // Configure Z80 data bus D0-D7 (PA0-PA7) as input with pull-up
  PORTA = 0xFF;

  digitalWrite(ETH_CS_, HIGH);                                                // ETH CS not active
  pinMode(ETH_CS_, OUTPUT);
  digitalWrite(MOSI, LOW);
  pinMode(MOSI, OUTPUT);
  pinMode(SCK, OUTPUT);
  pinMode(MISO, INPUT);

  // Initialize INT_, RAM_OE_DIS, and BUSREQ_
  pinMode(INT_, INPUT_PULLUP);                                                // Configure INT_ and set it NOT ACTIVE
  pinMode(INT_, OUTPUT);
  digitalWrite(INT_, HIGH);
  pinMode(RAM_OE_DIS, OUTPUT);                                                // Configure RAM_OE_DIS as output
  digitalWrite(RAM_OE_DIS, HIGH);                                             // Set RAM_OE_DIS active (ram read disabled)
  pinMode(WAIT_, INPUT_PULLUP);                                               // Configure WAIT_ as input (note: pullup prevents stall if WAIT flip flop is not driving WAIT signal)
  digitalWrite(BUSREQ_, HIGH);
  pinMode(BUSREQ_, OUTPUT);                                                   // Set BUSREQ_ HIGH

  // Initialize D0-D7, AD0, MREQ_, RD_ and WR_
  pinMode(MREQ_, INPUT_PULLUP);                                               // Configure MREQ_ as input with pull-up
  pinMode(RD_, INPUT_PULLUP);                                                 // Configure RD_ as input with pull-up
  pinMode(WR_, INPUT_PULLUP);                                                 // Configure WR_ as input with pull-up
  pinMode(AD0, INPUT_PULLUP);

  pinMode(CLK, OUTPUT);                                                       // Set CLK as output

  pinMode(SEL_WDOG, OUTPUT);                                                  // SELECT button / hardware watchdog trigger

  pinMode(BUSACK_, INPUT_PULLUP);                                             // BUSACK Z80

  digitalWrite(RAM_OE_DIS, LOW);                                              // Set RAM_OE_DIS inactive (ram read enabled)

  // Read the Z80 CPU speed mode
  clockDivider = EEPROM.read(EE_CLOCKDIVIDER_ADDR);                           // Read the previous stored value
  if (clockDivider == 0xFF)
    clockDivider = 1;                                                         // default 4 / 5MHz
  uint8_t maxDivider = (F_CPU/(2*Z80_MIN_CLOCK))-1;                           // F_CPU / (2* min_freq(Hz)) -1 = divider
  if (clockDivider > maxDivider)                                              // Z80 CLK not below 'Z80_MIN_CLOCK'
    clockDivider = maxDivider;

  // Read the stored Disk Set. If not valid set it to 0
  diskSet = EEPROM.read(EE_DISKSET_ADDR);
  if (diskSet >= maxDiskSet) 
  {
    EEPROM.update(EE_DISKSET_ADDR, 0);
    diskSet = 0;
  }

  // Read the Z80 watchdog-reset counter and date/time
  EEPROM.get(EE_Z80WATCHDOG_ADDR, Z80wdog_counters);
  if (Z80wdog_counters.count == 0xFFFFFFFF)
    Z80wdog_counters.count = 0;

  uint32_t baudrate = baudIndex[EEPROM.read(EE_SERBAUD_ADDR)];
  Serial.begin(baudrate);
  delay(500);

  // Initialize the I2C bus, 400kHz
  Wire.begin();
  Wire.setClock(400000);

  // Initialize the I2C display and print board info
  disp_ok = sh1106_init();
  if (disp_ok)
  {
    char dispP[22];

    sh1106_clear();
    sh1106_registerFont(&font6x8);

    sprintf(dispP, "%s - %s", HW_REV, IO_SUBS_END);
    sh1106_write_string(0, 0, dispP);

    sprintf(dispP, "Baudrate  %lu", baudrate);
    sh1106_write_string(0, 1, dispP);

    sprintf(dispP, "Z80 Wdog  %lu", Z80wdog_counters.count);
    sh1106_write_string(0, 3, dispP);
    if (Z80wdog_counters.count > 0)
    {
      sprintf(dispP, "(%02u/%02u/%02u %02u:%02u:%02u)",
          Z80wdog_counters.day, Z80wdog_counters.month, Z80wdog_counters.year,
          Z80wdog_counters.hours, Z80wdog_counters.minutes, Z80wdog_counters.seconds);
      sh1106_write_string(0, 4, dispP);
    }

    sh1106_write_string(0, 6, "Build");
    sh1106_write_string(0, 7, __DATE__);
    sh1106_write_string(80, 7, __TIME__);
  }
  
  // Initialize the EXP_PORT (I2C) and search for "known" optional modules
  Wire.beginTransmission(GPIOEXP_ADDR);
  if (Wire.endTransmission() == 0) 
  // Found GPE expansion
  {
    moduleGPIO = 1;                                                           // Set to 1 if GPIO Module is found
    // Set pullup enabled for GPA0 and GPA2:
    //  this just in case the SPP adapter is used and a printer is connected and powered on before the Z80-MBC2,
    //  to avoid a possible "strange" behavior of the printer (GPA0 = STROBE_, GPA2 = INIT_. See SPP Adapter schematic)
    Wire.beginTransmission(GPIOEXP_ADDR);
    Wire.write(GPPUA_REG);                        // Select GPPUA
    Wire.write(0b00000101);                       // Write value (1 = pullup enabled, 0 = pullup disabled)
    Wire.endTransmission();
  }

  // Check the serial speed index and set it to the default if needed
  if (EEPROM.read(EE_SERBAUD_ADDR) >= maxBaudIndex)
  // Invalid value. Set it to the default index
  {
    EEPROM.update(EE_SERBAUD_ADDR, maxBaudIndex-1);
  }

  // Try to muont the SD volume
  mountSD(&filesysSD); mountSD(&filesysSD);

  // ethernet init (sets 'eth_ok')
  w5500_check();
  if (eth_ok)
  {
    LoadNetworkConfig();
    w5500_init();
  }

  // Check if RTC is present and initialize it as needed
  RTCCheck(&rtcData);
  if (rtcData.foundRTC)
    readRTC(&rtcData);

  sysMenu(0);
}


// ------------------------------------------------------------------------------
//  SYSTEM MENU
//  Parameter: 0 = cold boot
// ------------------------------------------------------------------------------
void sysMenu(uint8_t bootm)
{
// ------------------------------------------------------------------------------
//  Local variables
// ------------------------------------------------------------------------------

  char          minBootChar     = '1';      // Minimum allowed ASCII value selection (boot selection)
  char          maxSelChar      = '9'+1;    // Maximum allowed ASCII value selection (boot selection)
  byte          maxBootMode     = 4;        // Default maximum allowed value for bootMode [0..4]
  byte          userKeyPressed  = 0;        // Flag to enter into the boot mode selection
  byte          rememberLastSel = 0;

//
// Performs some variables cleanup (when called from telnet, this menu should act like in a cold reset)
//
  Z80IntRx          = 0;
  Z80IntSysTick     = 0;
  sysTickTime       = 100;                                                    // default 100ms INT tick
  irqSafeBank       = 0xFF;                                                   // IRQ can be issued on any bank
  ioOpcode          = 0xFF;
  SPPmode           = 0;
  SPPautofd         = 0;
  BootImageSize     = 0;
  irqStatus         = 0;
  irqPending        = 0;
  sysTickCnt        = 0;
  tick100ms         = 0;
  tick1s            = 0;
  RxDoneFlag        = 1;
  cpmWarmBootFlg    = 0;
  telnetSpecialCmds = 0;
  ethTick           = 0;
  Z80WatchDOG_time  = 0;

  inTheMenu = 1;                                                              // indicates that the menu is running

  digitalWrite(RESET_, LOW);                                                  // Activate the Z80 RESET_ signal
  digitalWrite(INT_, HIGH);

  // disconnect TIMER2 from Z80 CLK for warm reset
  #if defined(__AVR_ATmega32__)
    // Atmega32 MCU
    TCCR2 &= ~((1 << COM21) | (1 << COM20));                                  // Disconnect OC2A (gpio mode on CLK)
  #elif defined(__AVR_ATmega1284__) || defined(__AVR_ATmega1284P__)
    // Atmega1284 MCU
    TCCR2A &= ~((1 << COM2A1) | (1 << COM2A0));                               // Disconnect OC2A (gpio mode on CLK)
  #else
    Serial.print(F("IOS: Current MCU is not supported. Aborted!"));
    while (1);
  #endif  

  digitalWrite(CLK, LOW);
  // Initialize CLK (single clock pulses mode) and reset the Z80 CPU
  pinMode(CLK, OUTPUT);                                                       // Set CLK as output
  singlePulsesResetZ80();                                                     // Reset the Z80 CPU using single clock pulses

  // Initialize the Logical RAM Bank (32KB) to map into the lower half of the Z80 addressing space
  currentBank = EEPROM.read(EE_DEFBANK_ADDR);
  if (currentBank > MAX_RAM_BANK)
    currentBank = DEFAULT_RAM_BANK;
  SetRAMBank(currentBank);

  // Check USER Key for boot mode changes 
  pinMode(USER, INPUT_PULLUP);                                                // Read USER Key to enter into the boot mode selection
  if (!digitalRead(USER)) userKeyPressed = 1;
  pinMode(USER, OUTPUT);                                                      // USER led OFF
  digitalWrite(USER, HIGH);

  rememberLastSel = EEPROM.read(EE_REMEMBERLASTSEL_ADDR);
  if (rememberLastSel > 1)
    rememberLastSel = 0;

   autoexecFlag = EEPROM.read(EE_AUTOEXECFLAG_ADDR);                          // Read the previous stored AUTOEXEC flag
   if (autoexecFlag > 1)
     autoexecFlag = 0;

  // ----------------------------------------
  // BOOT SELECTION AND SYS PARAMETERS MENU
  // ----------------------------------------

  if (rememberLastSel)                                                        // if option is enabled,
    bootMode = EEPROM.read(EE_BOOTMODE_ADDR);                                 // Read the previous stored boot mode
  else
    bootMode = 255;                                                           // otherwise invalidate the value to display the menu

  if ((userKeyPressed == 1 ) || (bootMode > maxBootMode) || bootm == 1)
  // Enter in the boot selection menu if USER key was pressed at startup
  //   or menu invoked from telnet management socket
  //   or an invalid bootMode code was read from internal EEPROM
  {
    uint8_t inCh;
    consolePrint("IOS: Select boot mode or system parameters:\r\n");
    do
    {
      // Print some system information
      consolePrint("\r\n\nZ80_MBC2-V - %s\r\nIOS - I/O Subsystem - %s-%s\r\n\r\n", HW_REV, IO_SUBS_BEGIN, IO_SUBS_END);

      // Print if the input serial buffer is 128 bytes wide (this is needed for xmodem protocol support)
      if (SERIAL_RX_BUFFER_SIZE >= 128) consolePrint("IOS: Found extended serial Rx buffer (%u bytes)\r\n", SERIAL_RX_BUFFER_SIZE);

      // Print the Z80 clock speed
      float freq = (float)F_CPU/(2000000L*(clockDivider+1));
      consolePrint("IOS: Z80 clock set at %d.%02d MHz\r\n", (int)freq, (int)((freq - (int)freq) * 100));

      // Print RTC and GPIO informations if found
      if (rtcData.foundRTC)
      {
        consolePrint("IOS: Found RTC DS3231 Module (");
        printDateTime(&rtcData);
        consolePrint(" T=%dC)\r\n", (int8_t)rtcData.tempC);

        if (rtcData.OscStopFlag)
          consolePrint("IOS: RTC CLOCK POWER FAILED: DATA IS UNRELIABLE!\r\n");
      }
      
      if (moduleGPIO) consolePrint("IOS: Found GPE Option\r\n");

      // Print ethernet info if found
      if (eth_ok)
      {
        consolePrint("IOS: Found W5500 ethernet controller (%u.%u.%u.%u)\r\n",
            netcfg.ip[0], netcfg.ip[1], netcfg.ip[2], netcfg.ip[3]);
      }

      // Print CP/M Autoexec on cold boot status
      consolePrint("IOS: CP/M Autoexec is ");
      if (autoexecFlag)
        consolePrint("ON");
      else
        consolePrint("OFF");

      consolePrint("\r\nIOS: Remember last Menu Selection is ");
      if (rememberLastSel) consolePrint("ON");
      else consolePrint("OFF");

      consolePrint("\r\nIOS: Z80 watchdog reset counter: %lu", Z80wdog_counters.count);
      if (Z80wdog_counters.count == 0)
        consolePrint("\r\n");
      else
        consolePrint(" (%02u/%02u/%02u %02u:%02u:%02u)\r\n",
            Z80wdog_counters.day, Z80wdog_counters.month, Z80wdog_counters.year,
            Z80wdog_counters.hours, Z80wdog_counters.minutes, Z80wdog_counters.seconds);

      FlushSerials();                                                         // flush serial (always) and telnet (if bridge is enabled)

      consolePrint("\r\n");
      if (bootMode <= maxBootMode)
      // Previous valid boot mode read, so enable '0' selection
      {
        minBootChar = '0';
        consolePrint(" 0: Recall Last Selection (%u)\r\n", bootMode + 1);
      }
      consolePrint(" 1: Basic\r\n");
      consolePrint(" 2: Forth\r\n");
      consolePrint(" 3: Load/set OS ");
      printOsName(diskSet);
      consolePrint("\r\n 4: Autoboot\r\n");
      consolePrint(" 5: iLoad\r\n");
      consolePrint(" 6: Change Z80 clock speed\r\n");
      consolePrint(" 7: Toggle CP/M Autoexec   (->");
      if (!autoexecFlag) consolePrint("ON");
      else consolePrint("OFF");
      consolePrint(")\r\n");
      consolePrint(" 8: Set serial port speed  (%lu)\r\n", baudIndex[EEPROM.read(EE_SERBAUD_ADDR)]);
      consolePrint(" 9: Set Initial RAM Bank   (%u)", currentBank);
      if (currentBank != DEFAULT_RAM_BANK)
        consolePrint(" (WARNING: default is %u)", DEFAULT_RAM_BANK);
      consolePrint("\r\n");
      consolePrint(" A: Toggle Remember Sel.   (->");
      if (!rememberLastSel) consolePrint("ON)");
      else consolePrint("OFF)");
      consolePrint("\r\n");
      // If RTC module is present add a menu choice
      if (rtcData.foundRTC)
      {
        consolePrint(" B: Set RTC time/date\r\n");
        maxSelChar = '9'+2;
      }
      if (eth_ok)
      {
        consolePrint(" C: Change Network Settings\r\n");
        maxSelChar = '9'+3;
      }

      // Ask a choice
      consolePrint("\r\nEnter your choice >");
      do
      {
        inChar = WaitAndBlink(CHECK);
        if (inChar >= 'a')                                                    // case insensitive
          inChar -= ('a' - 'A');
        if (inChar >= 'A')                                                    // above '9': make it contiguous (8...9...:...)
          inChar -= ('A' - ':');
      }               
      while (((inChar < minBootChar) || (inChar > maxSelChar)) &&
          inChar != 13);

      if (disp_ok)
        sh1106_contrast(0);                                                   // spegne il display

      if (inChar != 13)
        consolePrint("%c  Ok\r\n", inChar);

      // Make the selected action for the system parameters choice
      switch (inChar)
      {
        case '3':                                                             // Load/change current Disk Set
          printMsg1();
          iCount = (byte) (diskSet - 1);                                      // Set the previous Disk Set
          do
          {
            // Print the OS name of the next Disk Set
            iCount = (byte)(iCount + 1) % maxDiskSet;
            consolePrint("\r ->");
            printOsName(iCount);
            consolePrint("                 \r");
            FlushSerials();                                                   // flush serial (always) and telnet (if bridge is enabled)
            inCh = WaitAndBlink(BLK);                                         // Wait a key
          }
          while ((inCh != 13) && (inCh != 27));                               // Continue until a CR or ESC is pressed
          consolePrint("\r\n\r\n");
          if (inCh == 13)                                                     // Set and store the new Disk Set if required
          {
             diskSet = iCount;
             EEPROM.update(EE_DISKSET_ADDR, iCount);

            if (currentBank != 0)
            {
              consolePrint("Overriding selected bank (%u) to 0\r\n\r\n", currentBank);
              currentBank = 0;
              SetRAMBank(0);
            }
          }
          else                                                                // if ESC was pressed,
            inChar = 13;                                                      // stay in the menu
        break;
      
        case '6':                                                             // Change the clock speed of the Z80 CPU
          ChangeZ80Clock();
        break;

        case '7':                                                             // Toggle CP/M AUTOEXEC execution on cold boot
          autoexecFlag = !autoexecFlag;                                       // Toggle AUTOEXEC executiont status
          EEPROM.update(EE_AUTOEXECFLAG_ADDR, autoexecFlag);                  // Save it to the internal EEPROM
        break;

        case '8':                                                             // Change serial port speed
          printMsg1();
          iCount = EEPROM.read(EE_SERBAUD_ADDR);                              // Read the serial speed index
          iCount = (byte) (iCount - 1);                                       // Set the previous speed index
          do
          {
            // Print the current serial speed
            iCount = (byte)(iCount + 1) % maxBaudIndex;
            consolePrint("\r ->%lu   \r", baudIndex[iCount]);
            FlushSerials();                                                   // flush serial (always) and telnet (if bridge is enabled)
            inCh = WaitAndBlink(BLK);                                         // Wait a key
          }
          while ((inCh != 13) && (inCh != 27));                               // Continue until a CR or ESC is pressed
          consolePrint("\r\n\r\n");
          if ((inCh == 13) && (iCount != EEPROM.read(EE_SERBAUD_ADDR)))
          // Store new serial speed index
          {
            EEPROM.update(EE_SERBAUD_ADDR, iCount);
            consolePrint("Changed speed will be effective after next reboot!\r\n\r\n\n");
          }

        break;

        case '9':                                                             // Change default RAM bank
          ChangeDefaultBANK();
        break;

        case '9'+1:                                                           // = A = Toggle remember last selection
          rememberLastSel = !rememberLastSel;
          EEPROM.update(EE_REMEMBERLASTSEL_ADDR, rememberLastSel);            // Save it to the internal EEPROM
        break;

        case '9'+2:                                                           // = B = Change RTC Date/Time
          ChangeRTC();
        break;

        case '9'+3:                                                           // = C = Change network settings
          if (ChangeNetworkConfig())
          {
            telnet_handler();                                                 // if something changed, flushes last messages to client
            w5500_init();                                                     // and restart the interface
          }
        break;
      }
    }
    while(inChar >= '6' || inChar == 13);

    bootMode = inChar - '1';                                                  // Calculate bootMode from inChar

    if (rememberLastSel)
    {
      // Save selectd boot program if changed
      if (bootMode <= maxBootMode) EEPROM.update(EE_BOOTMODE_ADDR, bootMode); // Save to the internal EEPROM if required
      else bootMode = EEPROM.read(EE_BOOTMODE_ADDR);                          // Reload boot mode if '0' or > '5' choice selected
    }
  }

  // Print current Disk Set and OS name (if OS boot is enabled)
  if (bootMode == 2)
  {
    consolePrint("IOS: Current ");
    printOsName(diskSet);
    consolePrint("\r\n");
  }

  // ----------------------------------------
  // Z80 PROGRAM LOAD
  // ----------------------------------------

  digitalWrite(WAIT_RES, HIGH);

  // Get the starting address of the program to load and boot, and its size if stored in the flash
  switch (bootMode)
  {
    case 0:                                                                   // Load Basic from SD
      fileNameSD = BASICFN;
      BootStrAddr = BASSTRADDR;
      Z80IntRx = 1;                                                           // Enable Z80 Rx INT_ interrupt signal generation (Z80 M1 INT I/O)
    break;
    
    case 1:                                                                   // Load Forth from SD
      fileNameSD = FORTHFN;
      BootStrAddr = FORSTRADDR;
    break;

    case 2:                                                                   // Load an OS from current Disk Set on SD
      switch (diskSet)
      {
      case 0:                                                                 // CP/M 2.2
        fileNameSD = CPMFN;
        BootStrAddr = CPMSTRADDR;
      break;

      case 1:                                                                 // QP/M 2.71
        fileNameSD = QPMFN;
        BootStrAddr = QPMSTRADDR;
      break;

      case 2:                                                                 // CP/M 3.0
        fileNameSD = CPM3FN;
        BootStrAddr = CPM3STRADDR;
      break;

      case 3:                                                                 // UCSD Pascal
        fileNameSD = UCSDFN;
        BootStrAddr = UCSDSTRADDR;
      break;

      case 4:                                                                 // Collapse OS
        fileNameSD = COSFN;
        BootStrAddr = COSSTRADDR;
      break;

      case 5:                                                                 // Fuzix OS
        fileNameSD = FUZIXFN;
        BootStrAddr = FUZSTRADDR;
        Z80IntRx = 1;                                                         // Enable Z80 Rx INT_ interrupt signal generation (Z80 M1 INT I/O)
        Z80IntSysTick = 0; // <--- @@@  WAS 1                                 // Enable Z80 Systick INT_ interrupt signal generation (Z80 M1 INT I/O)
      break;
      }
    break;

    case 3:                                                                   // Load AUTOBOOT.BIN from SD (load an user executable binary file)
      fileNameSD = AUTOFN;
      BootStrAddr = AUTSTRADDR;
    break;

    case 4:                                                                   // Load iLoad from flash
      BootImage = (byte *) pgm_read_word (&flahBootTable[0]); 
      BootImageSize = sizeof(boot_A_);
      BootStrAddr = boot_A_StrAddr;
    break;
  }
  digitalWrite(WAIT_RES, LOW);                                                // Set WAIT_RES LOW (inactive, Led IOS ON)

  // Load a JP instruction if the boot program starting addr is > 0x0000
  if (BootStrAddr > 0x0000)                                                   // Check if the boot program starting addr > 0x0000
  // Inject a "JP <BootStrAddr>" instruction to jump at boot starting address
  {
    loadHL(0x0000);                                                           // HL = 0x0000 (used as pointer to RAM)
    loadByteToRAM(JP_nn);                                                     // Write the JP instruction @ 0x0000;
    loadByteToRAM(lowByte(BootStrAddr));                                      // Write LSB to jump @ 0x0001
    loadByteToRAM(highByte(BootStrAddr));                                     // Write MSB to jump @ 0x0002

#if DEBUG > 1
    //
    // DEBUG ----------------------------------
      Serial.print(F("DEBUG: Injected JP 0x"));
      Serial.println(BootStrAddr, HEX);
    // DEBUG END ------------------------------
    //
#endif    
  }

  // Execute the load of the selected file on SD or image on flash
  loadHL(BootStrAddr);                                                        // Set Z80 HL = boot starting address (used as pointer to RAM);

#if DEBUG > 1
  //
  // DEBUG ----------------------------------
    Serial.print(F("DEBUG: Flash BootImageSize = "));
    Serial.println(BootImageSize);
    Serial.print(F("DEBUG: BootStrAddr = "));
    Serial.println(BootStrAddr, HEX);    
    Serial.print(F("DEBUG: bootMode = "));
    Serial.println(bootMode);
    Serial.print(F("DEBUG: maxBootMode = "));
    Serial.println(maxBootMode);
  // DEBUG END ------------------------------
  //
#endif

  if (bootMode < maxBootMode)
  // Load from SD
  {
    // Mount a volume on SD
    if (mountSD(&filesysSD))
    // Error mounting. Try again
    {
      errCodeSD = mountSD(&filesysSD);
      if (errCodeSD)
      // Error again. Repeat until error disappears (or the user forces a reset)
      do
      {
        printErrSD(0, errCodeSD, NULL);
        waitKey();                                                            // Wait a key to repeat
        mountSD(&filesysSD);                                                  // New double try
        errCodeSD = mountSD(&filesysSD);
      }
      while (errCodeSD);
    }

    // Open the selected file to load
    errCodeSD = openSD(fileNameSD);
    if (errCodeSD)
    // Error opening the required file. Repeat until error disappears (or the user forces a reset)
    do
    {
      printErrSD(1, errCodeSD, fileNameSD);
      waitKey();                                                              // Wait a key to repeat
      errCodeSD = openSD(fileNameSD);
      if (errCodeSD != 3)
      // Try to do a two mount operations followed by an open
      {
        mountSD(&filesysSD);
        mountSD(&filesysSD);
        errCodeSD = openSD(fileNameSD);
      }
    }
    while (errCodeSD);

    // Read the selected file from SD and load it into RAM until an EOF is reached
    consolePrint("IOS: Loading boot program (%s)...", fileNameSD);

    telnet_handler();                                                         // flushes FIFO text to telnet client before entering the SD loop

    do
    // If an error occurs repeat until error disappears (or the user forces a reset)
    {
      do
      // Read a "segment" of a SD sector and load it into RAM
      {
        errCodeSD = readSD(bufferSD, &numReadBytes);                          // Read current "segment" (32 bytes) of the current SD serctor
        for (iCount = 0; iCount < numReadBytes; iCount++)
        // Load the read "segment" into RAM
        {
          loadByteToRAM(bufferSD[iCount]);                                    // Write current data byte into RAM
        }
      }
      while ((numReadBytes == 32) && (!errCodeSD));                           // If numReadBytes < 32 -> EOF reached
      if (errCodeSD)
      {
        printErrSD(2, errCodeSD, fileNameSD);
        waitKey();                                                            // Wait a key to repeat
        seekSD(0);                                                            // Reset the sector pointer
      }
    }
    while (errCodeSD);
  }
  else
  // Load from flash
  {
    consolePrint("IOS: Loading boot program...");
    for (word i = 0; i < BootImageSize; i++)
    // Write boot program into external RAM
    {
      loadByteToRAM(pgm_read_byte(BootImage + i));                            // Write current data byte into RAM
    }
  }
  consolePrint(" Done\r\n");

  // ----------------------------------------
  // Z80 BOOT
  // ----------------------------------------

  digitalWrite(RESET_, LOW);                                                  // Activate the RESET_ signal

  #if defined(__AVR_ATmega32__)
    // Atmega32 MCU
    // Z80 clock_freq = (Atmega_clock) / ((OCR2 + 1) * 2)
    ASSR &= ~(1 << AS2);                                                      // Set Timer2 clock from system clock
    TCCR2 |= (1 << CS20);                                                     // Set Timer2 clock to "no prescaling"
    TCCR2 &= ~((1 << CS21) | (1 << CS22));
    TCCR2 |= (1 << WGM21);                                                    // Set Timer2 CTC mode
    TCCR2 &= ~(1 << WGM20);
    TCCR2 |= (1 <<  COM20);                                                   // Set "toggle OC2 on compare match"
    TCCR2 &= ~(1 << COM21);
    OCR2 = clockDivider;                                                      // Set the compare value to toggle OC2 (0 = low or 1 = high)
  #elif defined(__AVR_ATmega1284__) || defined(__AVR_ATmega1284P__)
    // Atmega1284 MCU
    // Z80 clock_freq = (Atmega_clock) / ((OCR2 + 1) * 2)
    ASSR &= ~(1 << AS2);                                                      // Set Timer2 clock from system clock
    TCCR2B |= (1 << CS20);                                                    // Set Timer2 clock to "no prescaling"
    TCCR2B &= ~((1 << CS21) | (1 << CS22));
    TCCR2A |= (1 << WGM21);                                                   // Set Timer2 CTC mode
    TCCR2A &= ~(1 << WGM20);
    TCCR2A |= (1 <<  COM2A0);                                                 // Set "toggle OC2 on compare match"
    TCCR2A &= ~(1 << COM2A1);
    OCR2A = clockDivider;                                                     // Set the compare value to toggle OC2 (0 = low or 1 = high)
  #else
    Serial.print(F("IOS: Current MCU is not supported. Aborted!"));
    while (1);
  #endif

  pinMode(CLK, OUTPUT);                                                       // Set OC2 as output and start to output the clock
  consolePrint("IOS: Z80 is running from now\r\n\r\n");

  // Display a message (Z80_MBC2-V)
/*
  u8g2.setFont(u8g2_font_8x13_tf);
  u8g2.drawStr(0,30,"Z80 Running");
  u8g2.sendBuffer();
*/

  telnet_handler();                                                           // flushes FIFO text to telnet client
  FlushSerials();

//
// TIMER1 for 10ms tick generation
//
  TCCR1A = 0;
//  TCCR1B = (1 << WGM12) | (1 << CS12);                                        // CTC, prescaler 256
  TCCR1B = (1 << WGM12)|(1 << CS11)|(1 << CS10);                              // CTC, prescaler 64
  OCR1A = OCR1A_VALUE;
  TIFR1 = (1 << OCF1A);                                                       // Clear match flag

  // Leave the Z80 CPU running
  delay(1);                                                                   // Just to be sure...
  digitalWrite(RESET_, HIGH);                                                 // Release Z80 from reset and let it run

  inTheMenu = 0;
}


// ----------------------------------------
// FLUSH SERIAL /TELNET BUFFERS
// ----------------------------------------
void FlushSerials(void)
{
  // Flush serial
  while (Serial.available() > 0) 
    Serial.read();

  // Flush telnet Rx buffer if bridge enabled
  if (telnet_sessions[0].telnet_flags & TFLAG_CON_BRIDGE)
    telnet_sessions[0].rx_tail = telnet_sessions[0].rx_head;
}


// ----------------------------------------
// SET CURRENT RAM BANK
// ----------------------------------------
void SetRAMBank(uint8_t bank)
{
  digitalWrite(RAM_OE_DIS, HIGH);                                             // Set RAM_OE_DIS active (ram read disabled)
  DDRA = 0xFF;                                                                // Configure Z80 data bus D0-D7 (PA0-PA7) as output
  PORTA = bank;                                                               // 'bank' on databus (bank selection, Set RAM Bank)
  pinMode(BANK_LATCH, OUTPUT);                                                // Configures BANK_LATCH as output
  digitalWrite(BANK_LATCH, HIGH);                                             // Set the latch transparent
  digitalWrite(BANK_LATCH, LOW);                                              // Locks bank latch (databus latched)
  DDRA = 0x00;                                                                // Configure Z80 data bus D0-D7 (PA0-PA7) as input with pull-up
  PORTA = 0xFF;
}


// ----------------------------------------
// PRINTS STRING ON SERIAL AND TELNET FIFO
// ----------------------------------------
void consolePrint(const char *fmt, ...)
{
  char buf[512];                                                              // maximum message size
  va_list args;
  va_start(args, fmt);
  int len = vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  Serial.print(buf);

  if (telnet_sessions[0].telnet_flags & TFLAG_CON_BRIDGE)
  {
    for (int i = 0; i < len; i++)
      telnet_tx_push(0, buf[i]);
  }
}


// ------------------------------------------------------------------------------

void loop() 
{
  if (!(WAIT_PORTIN & (1 << WAIT_PIN)))
  // I/O operaton requested
  {
    Z80watchdog_ioOper = true;

    if (!(WR_PORTIN & (1 << WR_PIN)))
    // I/O WRITE operation requested

    // ----------------------------------------
    // VIRTUAL I/O WRITE OPERATIONS ENGINE
    // ----------------------------------------

    {
      ioAddress = (AD0_PORTIN & (1 << AD0_PIN));  // this way ioAddress gets the bit weight of the pin, but does not matter
      ioData = PINA;                              // Read Z80 data bus D0-D7 (PA0-PA7)
      if (ioAddress)                              // Check the I/O address (only AD0 is checked!)
      // .........................................................................................................
      //
      // AD0 = 1 (I/O write address = 0x01). STORE Opcode.
      //
      // Store (write) an "I/O operation code" (Opcode) and reset the exchanged bytes counter.
      //
      // NOTE 1: An Opcode can be a write or read Opcode, if the I/O operation is read or write.
      // NOTE 2: the STORE Opcode operation must always precede an EXECUTE WRITE Opcode or EXECUTE READ Opcode 
      //         operation.
      // NOTE 3: For multi-byte read Opcode (as DATETIME) read sequentially all the data bytes without to send
      //         a STORE Opcode operation before each data byte after the first one.
      // .........................................................................................................
      //
      // Currently defined Opcodes for I/O write operations:
      //
      //   Opcode     Name            Exchanged bytes
      // -------------------------------------------------
      // Opcode 0x00  USER LED        1
      // Opcode 0x01  SERIAL TX       1
      // Opcode 0x03  GPIOA Write     1
      // Opcode 0x04  GPIOB Write     1
      // Opcode 0x05  IODIRA Write    1
      // Opcode 0x06  IODIRB Write    1
      // Opcode 0x07  GPPUA Write     1
      // Opcode 0x08  GPPUB Write     1
      // Opcode 0x09  SELDISK         1
      // Opcode 0x0A  SELTRACK        2
      // Opcode 0x0B  SELSECT         1  
      // Opcode 0x0C  WRITESECT       512
      // Opcode 0x0D  SETBANK         1
      // Opcode 0x0E  SETIRQ          1
      // Opcode 0x0F  SETTICK         1
      // Opcode 0x10  SETOPT          1
      // Opcode 0x11  SETSPP          1
      // Opcode 0x12  WRSPP           1
      // Opcode 0xFF  No operation    1
      //
      // Opcodes for MBC2-V
      // Opcode 0x40  SETIRQBANK      1           // set the INT-safe ram bank, 0xFF = all
      // Opcode 0x41  WRDISPCTRL      n           // set display control data (coordinates, etc)
      // Opcode 0x42  WRDISPDATA      n           // write display data (text)
      // Opcode 0x43  SELTELNETSOCKET 1           // set the current telnet socket (session) number
      // Opcode 0x44  SETTELNETFLAGS  2           // set telnet flags byte (SET MASK, RESET MASK) for currently selected socket
      // Opcode 0x45  TELNET TX S0    1           // sends a byte over telnet socket 0
      // Opcode 0x46  TELNET TX S1    1           // sends a byte over telnet socket 1
      // Opcode 0x47  TELNET TX S2    1           // RESERVED for socket 2
      // Opcode 0x48  TELNET TX S3    1           // RESERVED for socket 3
      // Opcode 0x49  GPIOASETRESET   2
      // Opcode 0x4A  GPIOBSETRESET   2
      // Opcode 0x4B  SETWATCHDOG     1
      //
      //
      //
      // Currently defined Opcodes for I/O read operations:
      //
      //   Opcode     Name            Exchanged bytes
      // -------------------------------------------------
      // Opcode 0x80  USER KEY        1
      // Opcode 0x81  GPIOA Read      1
      // Opcode 0x82  GPIOB Read      1
      // Opcode 0x83  SYSFLAGS        1
      // Opcode 0x84  DATETIME        7
      // Opcode 0x85  ERRDISK         1
      // Opcode 0x86  READSECT        512
      // Opcode 0x87  SDMOUNT         1
      // Opcode 0x88  ATXBUFF         1
      // Opcode 0x89  SYSIRQ          1
      // Opcode 0x8A  GETSPP          1
      //
      // Opcodes for MBC2-V
      // Opcode 0xC0  GETTELNETFLAGS  1           // get telnet flags for currently selected socket
      // Opcode 0xC1  GETBANK         1           // get the active bank number
      // Opcode 0xC2  GETTELNETSTAT03 1           // get telnet read only status bits for sockets 0...3
      // Opcode 0xC3  TELNET RX S0    1           // get a byte from telnet socket 0
      // Opcode 0xC4  TELNET RX S1    1           // get a byte from telnet socket 1
      // Opcode 0xC5  TELNET RX S2    1           // RESERVED for socket 2
      // Opcode 0xC6  TELNET RX S3    1           // RESERVED for socket 3
      // Opcode 0xC7  ETH REMOTE IP   5           // get the IP address and proxy state of the device connected to the selected socket (see SELTELNETSOCKET)
      //
      // Opcode 0xFF  No operation    1
      //
      // See the following lines for the Opcodes details.
      // 
      // .........................................................................................................     
      {
#if DEBUG > 0
        if (ioOpcode != 0xFF)
          consolePrint("[PROTO] WR OPCODE (OUT1), 0x%02X while ioOpcode = 0x%02X, ioByteCnt = %u\r\n", ioData, ioOpcode, ioByteCnt);
#endif
#if DEBUG > 3
        if (ioOpcode != 0xFF)
          digitalWrite(SEL_WDOG, HIGH);           // opcode write trigger
#endif

        ioOpcode = ioData;                        // Store the I/O operation code (Opcode)
        ioByteCnt = 0;                            // Reset the exchanged bytes counter
      }
      else
      // .........................................................................................................
      //
      // AD0 = 0 (I/O write address = 0x00). EXECUTE WRITE Opcode.
      //
      // Execute the previously stored I/O write Opcode with the current data.
      // The code of the I/O write operation (Opcode) must be previously stored with a STORE Opcode operation.
      // .........................................................................................................
      //
      {
        switch (ioOpcode)
        // Execute the requested I/O WRITE Opcode. The 0xFF value is reserved as "No operation".
        {
          case  0x00:
            // USER LED:      
            //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                              x  x  x  x  x  x  x  0    USER Led off
            //                              x  x  x  x  x  x  x  1    USER Led on
            
            if (ioData & 0x01) digitalWrite(USER, LOW); 
            else digitalWrite(USER, HIGH);
          break;

          case  0x01:
            // SERIAL TX:     
            //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                             D7 D6 D5 D4 D3 D2 D1 D0    ASCII char to be sent to serial / telnet
            
            Serial.write(ioData);

            if (eth_ok && (telnet_sessions[0].telnet_flags & TFLAG_CON_BRIDGE))
              telnet_tx_push(0, ioData);
          break;

          case  0x03:
            // GPIOA Write (GPE Option):
            //
            //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                             D7 D6 D5 D4 D3 D2 D1 D0    GPIOA value (see MCP23017 datasheet)
            
            gpioPortA_M = ioData;

            if (moduleGPIO) 
            {
              Wire.beginTransmission(GPIOEXP_ADDR);
              Wire.write(GPIOA_REG);              // Select GPIOA
              Wire.write(ioData);                 // Write value
              Wire.endTransmission();
            }
          break;
          
          case  0x04:
            // GPIOB Write (GPE Option): 
            //   
            //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                             D7 D6 D5 D4 D3 D2 D1 D0    GPIOB value (see MCP23017 datasheet)
            
            gpioPortB_M = ioData;

            if (moduleGPIO) 
            {
              Wire.beginTransmission(GPIOEXP_ADDR);
              Wire.write(GPIOB_REG);              // Select GPIOB
              Wire.write(ioData);                 // Write value
              Wire.endTransmission();
            }
          break;
          
          case  0x05:
            // IODIRA Write (GPE Option):
            //
            //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                             D7 D6 D5 D4 D3 D2 D1 D0    IODIRA value (see MCP23017 datasheet)
            
            if (moduleGPIO) 
            {
              Wire.beginTransmission(GPIOEXP_ADDR);
              Wire.write(IODIRA_REG);             // Select IODIRA
              Wire.write(ioData);                 // Write value
              Wire.endTransmission();
            }
          break;
          
          case  0x06:
            // IODIRB Write (GPE Option):
            //
            //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                             D7 D6 D5 D4 D3 D2 D1 D0    IODIRB value (see MCP23017 datasheet)
            
            if (moduleGPIO) 
            {
              Wire.beginTransmission(GPIOEXP_ADDR);
              Wire.write(IODIRB_REG);             // Select IODIRB
              Wire.write(ioData);                 // Write value
              Wire.endTransmission();
            }
          break;
          
          case  0x07:
            // GPPUA Write (GPE Option):
            //
            //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                             D7 D6 D5 D4 D3 D2 D1 D0    GPPUA value (see MCP23017 datasheet)
            
            if (moduleGPIO) 
            {
              Wire.beginTransmission(GPIOEXP_ADDR);
              Wire.write(GPPUA_REG);              // Select GPPUA
              Wire.write(ioData);                 // Write value
              Wire.endTransmission();
            }
          break;
          
          case  0x08:
            // GPPUB Write (GPIO Exp. Mod. ):
            //
            //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                             D7 D6 D5 D4 D3 D2 D1 D0    GPPUB value (see MCP23017 datasheet)
            
            if (moduleGPIO) 
            {
              Wire.beginTransmission(GPIOEXP_ADDR);
              Wire.write(GPPUB_REG);              // Select GPPUB
              Wire.write(ioData);                 // Write value
              Wire.endTransmission();
            }
          break;
          
          case  0x09:
            // DISK EMULATION
            // SELDISK - select the emulated disk number (binary). 100 disks are supported [0..99]:
            //
            //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                             D7 D6 D5 D4 D3 D2 D1 D0    DISK number (binary) [0..99]
            //
            //
            // Opens the "disk file" correspondig to the selected disk number, doing some checks.
            // A "disk file" is a binary file that emulates a disk using a LBA-like logical sector number.
            // Every "disk file" must have a dimension of 8388608 bytes, corresponding to 16384 LBA-like logical sectors
            //  (each sector is 512 bytes long), correspinding to 512 tracks of 32 sectors each (see SELTRACK and 
            //  SELSECT Opcodes).
            // Errors are stored into "errDisk" (see ERRDISK Opcode).
            //
            //
            // ...........................................................................................
            //
            // "Disk file" filename convention:
            //
            // Every "disk file" must follow the sintax "DSsNnn.DSK" where
            //
            //    "s" is the "disk set" and must be in the [0..9] range (always one numeric ASCII character)
            //    "nn" is the "disk number" and must be in the [00..99] range (always two numeric ASCII characters)
            //
            // ...........................................................................................
            //          
            //
            // NOTE 1: The maximum disks number may be lower due the limitations of the used OS (e.g. CP/M 2.2 supports
            //         a maximum of 16 disks)
            // NOTE 2: Because SELDISK opens the "disk file" used for disk emulation, before using WRITESECT or READSECT
            //         a SELDISK must be performed at first.

#if DEBUG > 2
            Serial.printf("[SELDISK] %u\r\n", ioData);
#endif

            if (ioData <= maxDiskNum)             // Valid disk number
            // Set the name of the file to open as virtual disk, and open it
            {
              diskName[2] = diskSet + 48;         // Set the current Disk Set
              diskName[4] = (ioData / 10) + 48;   // Set the disk number
              diskName[5] = ioData - ((ioData / 10) * 10) + 48;
              diskErr = openSD(diskName);         // Open the "disk file" corresponding to the given disk number
            }
            else diskErr = 16;                    // Illegal disk number

#if DEBUG > 2                                     // 1 (higher level) because may happen when a S.O scans for volumes
            if (diskErr)
              Serial.printf("[SELDISK] openSD %u error %u\r\n", ioData, diskErr);
#endif
          break;
  
          case  0x0A:
            // DISK EMULATION
            // SELTRACK - select the emulated track number (word splitted in 2 bytes in sequence: DATA 0 and DATA 1):
            //
            //                I/O DATA 0:  D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                             D7 D6 D5 D4 D3 D2 D1 D0    Track number (binary) LSB [0..255]
            //
            //                I/O DATA 1:  D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                             D7 D6 D5 D4 D3 D2 D1 D0    Track number (binary) MSB [0..1]
            //
            //
            // Stores the selected track number into "trackSel" for "disk file" access.
            // A "disk file" is a binary file that emulates a disk using a LBA-like logical sector number.
            // The SELTRACK and SELSECT operations convert the legacy track/sector address into a LBA-like logical 
            //  sector number used to set the logical sector address inside the "disk file".
            // A control is performed on both current sector and track number for valid values. 
            // Errors are stored into "diskErr" (see ERRDISK Opcode).
            //
            //
            // NOTE 1: Allowed track numbers are in the range [0..511] (512 tracks)
            // NOTE 2: Before a WRITESECT or READSECT operation at least a SELSECT or a SELTRAK operation
            //         must be performed
  
            if (!ioByteCnt)
            // LSB
            {
              trackSel = ioData;
            }
            else
            // MSB
            {
              trackSel = (((word) ioData) << 8) | lowByte(trackSel);
#if DEBUG > 2
              Serial.printf("[SELTRACK] %u\r\n", trackSel);
#endif
              if ((trackSel < 512) && (sectSel < 32))
              // Sector and track numbers valid
              {
                diskErr = 0;                      // No errors
              }
              else
              // Sector or track invalid number
              {
                if (sectSel < 32) diskErr = 17;   // Illegal track number
                else diskErr = 18;                // Illegal sector number
              }
              ioOpcode = 0xFF;                    // All done. Set ioOpcode = "No operation"

#if DEBUG > 1
              if (diskErr)
                Serial.printf("[SELTRACK] %u error %u\r\n", trackSel, diskErr);
#endif
            }
            ioByteCnt++;
          break;
  
          case  0x0B:
            // DISK EMULATION
            // SELSECT - select the emulated sector number (binary):
            //
            //                  I/O DATA:  D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                             D7 D6 D5 D4 D3 D2 D1 D0    Sector number (binary) [0..31]
            //
            //
            // Stores the selected sector number into "sectSel" for "disk file" access.
            // A "disk file" is a binary file that emulates a disk using a LBA-like logical sector number.
            // The SELTRACK and SELSECT operations convert the legacy track/sector address into a LBA-like logical 
            //  sector number used to set the logical sector address inside the "disk file".
            // A control is performed on both current sector and track number for valid values. 
            // Errors are stored into "diskErr" (see ERRDISK Opcode).
            //
            //
            // NOTE 1: Allowed sector numbers are in the range [0..31] (32 sectors)
            // NOTE 2: Before a WRITESECT or READSECT operation at least a SELSECT or a SELTRAK operation
            //         must be performed
  
#if DEBUG > 2
            Serial.printf("[SELSECT] %u\r\n", ioData);
#endif

            sectSel = ioData;
            if ((trackSel < 512) && (sectSel < 32))
            // Sector and track numbers valid
            {
              diskErr = 0;                        // No errors
            }
            else
            // Sector or track invalid number
            {
              if (sectSel < 32) diskErr = 17;     // Illegal track number
              else diskErr = 18;                  // Illegal sector number
            }

#if DEBUG > 1
            if (diskErr)
              Serial.printf("[SELSECT] %u error %u\r\n", ioData, diskErr);
#endif
          break;
  
          case  0x0C:
            // DISK EMULATION
            // WRITESECT - write 512 data bytes sequentially into the current emulated disk/track/sector:
            //
            //                 I/O DATA 0: D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                             D7 D6 D5 D4 D3 D2 D1 D0    First Data byte
            //
            //                      |               |
            //                      |               |
            //                      |               |                 <510 Data Bytes>
            //                      |               |
            //
            //
            //               I/O DATA 511: D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                             D7 D6 D5 D4 D3 D2 D1 D0    512th Data byte (Last byte)
            //
            //
            // Writes the current sector (512 bytes) of the current track/sector, one data byte each call. 
            // All the 512 calls must be always performed sequentially to have a WRITESECT operation correctly done. 
            // If an error occurs during the WRITESECT operation, all subsequent write data will be ignored and
            //  the write finalization will not be done.
            // If an error occurs calling any DISK EMULATION Opcode (SDMOUNT excluded) immediately before the WRITESECT 
            //  Opcode call, all the write data will be ignored and the WRITESECT operation will not be performed.
            // Errors are stored into "diskErr" (see ERRDISK Opcode).
            //
            // NOTE 1: Before a WRITESECT operation at least a SELTRACK or a SELSECT must be always performed
            // NOTE 2: Remember to open the right "disk file" at first using the SELDISK Opcode
            // NOTE 3: The write finalization on SD "disk file" is executed only on the 512th data byte exchange, so be 
            //         sure that exactly 512 data bytes are exchanged.
  
            if (!ioByteCnt)
            // First byte of 512, so set the right file pointer to the current emulated track/sector first
            {
#if DEBUG > 2
              Serial.println(F("[WRITESEC] start"));
#endif
              if ((trackSel < 512) && (sectSel < 32) && (!diskErr))
              // Sector and track numbers valid and no previous error; set the LBA-like logical sector
              {
                diskErr = seekSD((trackSel << 5) | sectSel);  // Set the starting point inside the "disk file"
                                                              //  generating a 14 bit "disk file" LBA-like 
                                                              //  logical sector address created as TTTTTTTTTSSSSS
#if DEBUG > 1
                if (diskErr)
                  Serial.printf("[WRITESECT] seekSD %u error %u\r\n", ((trackSel << 5) | sectSel), diskErr);
#endif
              }
            }
            
  
            if (!diskErr)
            // No previous error (e.g. selecting disk, track or sector)
            {
              byte tempByte = ioByteCnt % 32;     // [0..31]
              bufferSD[tempByte] = ioData;        // Store current exchanged data byte in the buffer array
              if (tempByte == 31)
              // Buffer full. Write all the buffer content (32 bytes) into the "disk file"
              {
                diskErr = writeSD(bufferSD, &numWriBytes);
                if (numWriBytes < 32) diskErr = 19; // Reached an unexpected EOF
#if DEBUG > 1
                if (diskErr)
                  Serial.printf("[WRITESECT] writeSD error %u\r\n", diskErr);
#endif
                if (ioByteCnt >= 511)
                // Finalize write operation and check result (if no previous error occurred)
                {
#if DEBUG > 2
                  Serial.println(F("[WRITESEC] end"));
#endif
                  if (!diskErr)
                  {
                    diskErr = writeSD(NULL, &numWriBytes);
#if DEBUG > 1
                    if (diskErr)
                      Serial.printf("[WRITESECT] finalize writeSD %u\r\n", diskErr);
#endif
                  }
                  ioOpcode = 0xFF;                // All done. Set ioOpcode = "No operation"
                }
              }
            }
            ioByteCnt++;                          // Increment the counter of the exchanged data bytes
          break;
  
          case  0x0D:
            // BANKED RAM
            // SETBANK - select the Os RAM Bank (binary), modified for Z80_MBC2-V:
            //
            //                  I/O DATA:  D7 D6 D5 D4 D3 D2 D1 D0
            //                            -----------------------------------------------------------
            //                                         D3 D2 D1 D0    Os Bank number (binary) [0..14]
            //
            //
            // Set a 32kB RAM bank for the lower half of the Z80 address space (from 0x0000 to 0x7FFF).
            // The upper half (from 0x8000 to 0xFFFF) is the common fixed bank.
            // Allowed Os Bank numbers are from 0 to 14.
            //
            // On Z80_MBC2-V a single latch signal is used. The 4 LSB bit from Z80 databus are latched.
            //
            // An OR array forces the upper physical bank (15, 0x0F) when A15 is high.
            //
            //
            //  Os Bank |  Z80 Address Bus    |   Physical Bank                   |   Physical RAM Addresses   |            Notes
            //  number  |        A15          |  RAM_A18 RAM_A17 RAM_A16 RAM_A15  |                            |
            // -----------------------------------------------------------------------------------------------------------------------------
            //     X    |         1           |     1       1      1       1      |  From 0x78000 to 0x7FFFF   |  Phy Bank 15 (common fixed)
            //     0    |         0           |     0       0      0       0      |  From 0x00000 to 0x07FFF   |  Phy Bank  0
            //     1    |         0           |     0       0      0       1      |  From 0x08000 to 0x0FFFF   |  Phy Bank  1
            //     2    |         0           |     0       0      1       0      |  From 0x10000 to 0x17FFF   |  Phy Bank  2
            //     3    |         0           |     0       0      1       1      |  From 0x18000 to 0x1FFFF   |  Phy Bank  3
            //     4    |         0           |     0       1      0       0      |  From 0x20000 to 0x27FFF   |  Phy Bank  4
            //     5    |         0           |     0       1      0       1      |  From 0x28000 to 0x2FFFF   |  Phy Bank  5
            //     6    |         0           |     0       1      1       0      |  From 0x30000 to 0x37FFF   |  Phy Bank  6
            //     7    |         0           |     0       1      1       1      |  From 0x38000 to 0x3FFFF   |  Phy Bank  7
            //     8    |         0           |     1       0      0       0      |  From 0x40000 to 0x47FFF   |  Phy Bank  8
            //     9    |         0           |     1       0      0       1      |  From 0x48000 to 0x4FFFF   |  Phy Bank  9
            //    10    |         0           |     1       0      1       0      |  From 0x50000 to 0x57FFF   |  Phy Bank 10
            //    11    |         0           |     1       0      1       1      |  From 0x58000 to 0x5FFFF   |  Phy Bank 11
            //    12    |         0           |     1       1      0       0      |  From 0x60000 to 0x67FFF   |  Phy Bank 12
            //    13    |         0           |     1       1      0       1      |  From 0x68000 to 0x6FFFF   |  Phy Bank 13
            //    14    |         0           |     1       1      1       0      |  From 0x70000 to 0x77FFF   |  Phy Bank 14
            //
            //
            //
            //      Physical Bank      |    Logical Bank     |   Physical Bank   |   Physical RAM Addresses
            //          number         |       number        |  RAM_A16 RAM_A15  |
            // ------------------------------------------------------------------------------------------------
            //            0            |         1           |     0       0     |   From 0x00000 to 0x07FFF 
            //            1            |         0           |     0       1     |   From 0x08000 to 0x0FFFF
            //            2            |         3           |     1       0     |   From 0x01000 to 0x17FFF
            //            3            |         2           |     1       1     |   From 0x18000 to 0x1FFFF
            //
            //
            // Note that the Logical Bank 0 can't be used as switchable Os Bank bacause it is the common 
            //  fixed bank mapped in the upper half of the Z80 address space (from 0x8000 to 0xFFFF).
            //
            //
            // NOTE: If the Os Bank number is greater than 14 (15?) no selection is done.

            if (ioData <= MAX_RAM_BANK)
            {
//              digitalWrite(BANK_LATCH, HIGH);                 
//              digitalWrite(BANK_LATCH, LOW);                  
              BANK_LATCH_PORT |= (1 << BANK_LATCH_PIN);       // Sets the latch transparent (databus capture)
              currentBank = ioData;                           // updates 'currentBank'
              BANK_LATCH_PORT &= ~(1 << BANK_LATCH_PIN);      // Locks bank latch (databus latched)
#if DEBUG > 2
              consolePrint("[SETBANK] %u\r\n", currentBank);
#endif
            }
            else
              consolePrint("[SETBANK] Set %u Ignored", ioData);
            ioOpcode = 0xFF;                                  // All done. Set ioOpcode = "No operation" (redundant)
          break;
  
          case  0x0E:
            // SETIRQ - enable/disable the IRQ generation
            //
            //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                              X  X  X  X  X  X  X  0    Serial Rx IRQ not enabled
            //                              X  X  X  X  X  X  X  1    Serial Rx IRQ enabled
            //                              X  X  X  X  X  X  0  X    Systick IRQ not enabled
            //                              X  X  X  X  X  X  1  X    Systick IRQ enabled
            //
            //
            // The <>>>>>>< signal is shared among various interrupt requests. This allows to use the simplified 
            //  Mode 1 scheme of the Z80 CPU (fixed jump to 0x0038 on INT_ signal active) with multiple interrupt causes.
            //
            // The SETIRQ purpose is to enable/disable the generation of an IRQ (using the INT_ signal)
            //  selecting wich event you want enable.
            //
            // When a IRQ is enabled you have to serve it on the Z80 side with a ISR (Interrupt Service Routine).
            //
            // Inside the ISR code, you have to read the SYSIRQ Opcode to know the exact causes of the interrupt (see the 
            //  SYSIRQ Opcode) because multiple causes/bits could be active, so your ISR must be written to check and 
            //  serve them all.
            //
            // NOTE 1: Only D0 and D1 are currently used.
            // NOTE 2: At reset time all the IRQ "triggers" (D7-D0) are normally disabled (unless they are enabled 
            //         for special boot cases).
            // 
            // ...................................................................................
            //
            // Note about the Z80 CPU interrupt signal generation (INT_ signal):
            //
            // The current version of IOS is designed to use the Interrupt Mode 1 of the Z80 CPU (when enabled). 
            // Using this mode an occuring interrupt will cause a jump to the fixed address 0x0038.
            // Therefore to know wich kind of interrupt was triggered you need to use the SYSIRQ Opcode inside the 
            //  ISR and store the result (the SYSIRQ Opcode resets his IRQ flags after every call) to jump to the 
            //  needed serving sub-routines.
            // 
            // 
            // Note about the Serial Rx interrupt signal generation (Z80 CPU): 
            // 
            // When enabled an interrupt is generated (INT_ signal LOW) when there is at least one character inside 
            //  the the serial RX buffer. When the Z80 CPU acknoledges the interrupt request an interrupt acknoledge bus 
            //  cycle is executed on the Z80 bus, and during this cycle the interrupt request signal is 
            //  reset (INT_ signal HIGH) by IOS.
            // At this point no further serial RX interrupt will be activated until a following serial Rx read I/O 
            //  operation is executed by the Z80 CPU.
            // 
            // 
            // Note about the Systick interrupt signal generation (Z80 CPU):
            // 
            // When enabled an interrupt is generated (INT_ signal LOW) every a given amount of time (default is 100ms).
            // To set/change the Systick time the SETTICK Opcode must be used.
            // When the Systick interrupt is acknoledged by the Z80 CPU with an interrupt acknoledge bus cycle, the 
            //  interrupt request signal is reset (INT_ signal HIGH) by IOS.
            // 
            // 
            // Note about the IOS Opcode calling sequence when interrupt signal generation is enabled:
            // 
            // When interrupt is enabled care must be taken when calling IOS Opcodes as they must be considered as an 
            //  atomic action (calling an Opcode requires at least two I/O operations, where the first one is used to 
            //  set the operation code).
            // To ensure safe Opcode calls inside the Z80 user code, before every Opcode call the interrupt must be 
            //  disabled an re-enabled soon after the completion of the Opcode call.
            // 
            // ...................................................................................

#if DEBUG > 1
            consolePrint("[SETIRQ] %u\r\n", ioData);
#endif
            Z80IntRx = ioData & 1;                      // Enable/disable the Systick IRQ generation
            Z80IntSysTick = (ioData & (1 << 1)) >> 1;   // Enable/disable the Serial Rx IRQ generation
          break;
  
          case  0x0F:
            // SETTICK - set the Systick timer time (milliseconds)
            //
            //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                             D7 D6 D5 D4 D3 D2 D1 D0    Systick time (binary) [1..255]
            //
            // Set/change the time used for the Systick timer.
            // At reset time the default value is 10 (100ms).
            // See SETIRQ and SYSIRQ Opcodes for more info.
            //
            // NOTE: If the time is 0, the set operation is ignored.

#if DEBUG > 1
            consolePrint("[SETTICK] %u\r\n", ioData);
#endif
            if (ioData >0) sysTickTime = ioData;
          break;

          case  0x10:
            // SETOPT - set system options
            //
            //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                              x  x  x  x  x  x  x  0    CP/M warm boot message disabled
            //                              x  x  x  x  x  x  x  1    CP/M warm boot message enabled
            //
            // Set/change some system options (currently only D0 is defined).
            // At reset time the default value for D0 is 0 (message disabled).
            // The value of D0 can be read using the SYSFLAGS Opcode (bit D4).
            //
            // NOTE: The CP/M custom BIOS (CBIOS) must implement a read of the SYSFLAGS (bit D4) Opcode to switch on/off
            //       the CP/M warm boot message display

            cpmWarmBootFlg = ioData & 0x01;
          break;
  
          case  0x11:
            // SPP EMULATION
            // SETSPP - set the GPIO port into SPP mode:     
            //
            //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                              x  x  x  x  x  x  x  0    AUTOFD disabled
            //                              x  x  x  x  x  x  x  1    AUTOFD enabled
            //
            // The SETSPP Opcode is used when an SPP Adapter board is connected to the GPIO port to work as 
            //  a Standard Parallel Port.
            //
            // The following actions are performed:
            // - The SPP mode flag is set;
            // - The GPIO port is set (direction and pullup) to operate as a SPP port;
            // - The STROBE (active low) Control Line of the SPP port is set to High;
            // - D0 is used to set the status of AUTOFD (active Low) Contro Line of the SPP port (AUTOFD = !D0);
            // - The printer is initialized with a pulse on the INIT (active Low) Control line of the SPP port.
            //
            // GPIO port / SPP port signals table:
            //
            //                       GPIO Port |  SPP Port              | Dir
            //                      -------------------------------------------
            //                         GPA0    |  STROBE (active Low)   | Out
            //                         GPA1    |  AUTOFD (active Low)   | Out
            //                         GPA2    |  INIT (active Low)     | Out
            //                         GPA3    |  ACK (active Low)      | In
            //                         GPA4    |  BUSY (active High)    | In
            //                         GPA5    |  PAPEREND (active High)| In
            //                         GPA6    |  SELECT (active High)  | In
            //                         GPA7    |  ERROR (active Low)    | In
            //                         GPB0    |  D0                    | Out
            //                         GPB1    |  D1                    | Out
            //                         GPB2    |  D2                    | Out
            //                         GPB3    |  D3                    | Out
            //                         GPB4    |  D4                    | Out
            //                         GPB5    |  D5                    | Out
            //                         GPB6    |  D6                    | Out
            //                         GPB7    |  D7                    | Out
            //
            // NOTE 1: When the GPIO is set to operate as an SPP port all the GPIO write Opcodes (GPIOA Write, GPIOB Write, IODIRA Write, 
            //         IODIRB Write, GPPUA Write, GPPUB Write) are ignored/disabled.
            // NOTE 2: If the GPIO expansion module (GPE) is not found this Opcode is ignored.
            // NOTE 3: When the SPP mode is activated cannot be disabled anymore (the only way is reset the board).
        
            if (moduleGPIO)                       // Only if GPE was found
            {
              SPPmode = 1;                        // Set the SPP mode flag
              SPPautofd = (!ioData) & 0x01;       // Store the value of the AUTOFD Control Line (active Low))
              
              // Set STROBE and INIT at 1, and AUTOFD = !D0
              Wire.beginTransmission(GPIOEXP_ADDR);
              Wire.write(GPIOA_REG);                                    // Select GPIOA
              Wire.write(0b00000101 | (byte) (SPPautofd << 1));         // Write value
              Wire.endTransmission();
              
              // Set the GPIO port to work as an SPP port (direction and pullup)
              Wire.beginTransmission(GPIOEXP_ADDR);
              Wire.write(IODIRA_REG);                                   // Select IODIRA
              Wire.write(0b11111000);                                   // Write value (1 = input, 0 = ouput)
              Wire.endTransmission();
              Wire.beginTransmission(GPIOEXP_ADDR);
              Wire.write(IODIRB_REG);                                   // Select IODIRB 
              Wire.write(0b00000000);                                   // Write value (1 = input, 0 = ouput)
              Wire.endTransmission();
              Wire.beginTransmission(GPIOEXP_ADDR);
              Wire.write(GPPUA_REG);                                    // Select GPPUA
              Wire.write(0b11111111);                                   // Write value (1 = pullup enabled, 0 = pullup disabled)
              Wire.endTransmission();
              
              // Initialize the printer using a pulse on INIT
              // NOTE: The I2C protocol introduces delays greater than needed by the SPP, so no further delay is used here to generate the pulse
              byte tempByte = 0b00000001 | (byte) (SPPautofd << 1);     // Change INIT bit to active (Low)
              Wire.beginTransmission(GPIOEXP_ADDR);
              Wire.write(GPIOA_REG);                                    // Select GPIOA
              Wire.write(tempByte);                                     // Set INIT bit to active (Low)
              Wire.endTransmission();
              tempByte = tempByte | 0b00000100;                         // Change INIT bit to not active (High)
              Wire.beginTransmission(GPIOEXP_ADDR);
              Wire.write(GPIOA_REG);                                    // Select GPIOA
              Wire.write(tempByte);                                     // Set INIT bit to not active (High)
              Wire.endTransmission();
            }
          break;  
  
          case  0x12:
            // SPP EMULATION
            // WRSPP - send a byte to the printer attached to the SPP port:     
            //
            //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                             D7 D6 D5 D4 D3 D2 D1 D0    byte to be sent to SPP
            //
            // If the SPP mode is enabled send a byte to the SPP. No check is done here to know if the printer is 
            //  ready or not, so you have to use the GETSPP Opcode before for that.
            // If the SPP mode is disabled (or the GPE is not installed) this Opcode is ignored.
            //
            // NOTE: to use WRSPP the SETSPP Opcode should be called first to activate the SPP mode of the GPIO port.
            
            if (SPPmode)                                                // Only if SPP mode is active
            {
              // NOTE: The I2C protocol introduces delays greater than needed by the SPP, so no further delay is used here to generate the pulse
              Wire.beginTransmission(GPIOEXP_ADDR);
              Wire.write(GPIOB_REG);                                    // Select GPIOB
              Wire.write(ioData);                                       // Data on GPIOB
              Wire.endTransmission();
              byte tempByte = 0b11111100 | (byte) (SPPautofd << 1);     // Change STROBE bit to active (Low)
              Wire.beginTransmission(GPIOEXP_ADDR);
              Wire.write(GPIOA_REG);                                    // Select GPIOA
              Wire.write(tempByte);                                     // Set STROBE bit to active (Low)
              Wire.endTransmission();
              tempByte = tempByte | 0b00000001;                         // Change STROBE bit to not active (High)
              Wire.beginTransmission(GPIOEXP_ADDR);
              Wire.write(GPIOA_REG);                                    // Select GPIOA
              Wire.write(tempByte);                                     // Set STROBE bit to not active (High)
              Wire.endTransmission();
            }
          break;

          case 0x40:
            // SETIRQBANK
            // Sets the bank number which is safe for INT assertion (255 = all banks)
            if ((ioData <= MAX_RAM_BANK) || (ioData == 255))
            {
              irqSafeBank = ioData;
#if DEBUG > 1
              consolePrint("[SETIRQBANK] %u\r\n", irqSafeBank);
#endif
            }
          break;

          case 0x41:
            // WRDISPCTRL
            // Set display's control variables
            //
            // Subcommand's codes:
            // 0x00 = set coordinates;
            // 0x01 = clear screen
            // 0x02 = set contrast
            //
            if (ioByteCnt < sizeof(bufferDISP))
            {
              *(bufferDISP+ioByteCnt) = ioData;
              ++ioByteCnt;
              
              if (ioByteCnt == 3 && *bufferDISP == 0)                   // subcommand 0 = set coordinates, 2 data bytes
              {
                disp_X = *(bufferDISP+1);
                disp_Y = *(bufferDISP+2);
                dispOp = DISPOP_SETPOS;                                 // 2 = set cursor position
                ioOpcode = 0xFF;                                        // All done. Set ioOpcode = "No operation"
              }
              if (ioByteCnt == 1 && *bufferDISP == 1)                   // subcommand 1 = clear screen
              {
                dispOp = DISPOP_CLEAR;                                  // clear screen operation request
                ioOpcode = 0xFF;                                        // All done. Set ioOpcode = "No operation"
              }
              if (ioByteCnt == 2 && *bufferDISP == 2)                   // subcommand 2 = set contrast
              {
                dispOp = DISPOP_SETCONTRAST;                            // set contrast operation request
                dispData = *(bufferDISP+1);
                ioOpcode = 0xFF;                                        // All done. Set ioOpcode = "No operation"
              }
            }
          break;

          case 0x42:
            // WRDISPDATA
            // Write char
            if (ioData != '\0')
            {
              dispData = ioData;
              dispOp = DISPOP_WRCHAR;                                   // draw text operation request
            }
            else
            {
              dispOp = DISPOP_SETPOS;                                   // NULL (end of string): reposition the cursor at given coordinates
              ioOpcode = 0xFF;                                          // All done. Set ioOpcode = "No operation"
            }
          break;

          case 0x43:
            // SELTELNETSOCKET
            if (ioData < MAX_TELNET_SESSIONS)
            {
              telnetSocketSel = ioData;
#if DEBUG > 0
              consolePrint("[SELTELNETSOCKET] %u\r\n", telnetSocketSel);
#endif
            }
            else
              Serial.printf("[SELTELSOCKET] Invalid socked %u\r\n", ioData);
          break;

          case 0x44:
            // SETTELNETFLAGS
            //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                              X  X  X  X  X  X  X  X    SET mask
            //                              X  X  X  X  X  X  X  X    RESET mask
            //
            if (telnetSocketSel < MAX_TELNET_SESSIONS)
            {
              if ((ioData & TFLAG_RAW_MODE) && telnet_sessions[telnetSocketSel].iac_state != 0)         // for both SET and RESET of 'TFLAG_RAW_MODE'
              {
                telnet_sessions[telnetSocketSel].iac_state = 0;
#if DEBUG > 0
                consolePrint("[SETTELNETFLAGS] iac_state cleared\r\n");
#endif
              }
              if (!ioByteCnt)                                                                           // first byte is the SET mask
              {
                telnet_sessions[telnetSocketSel].telnet_flags |= ioData;
#if DEBUG > 0
                consolePrint("[SETTELNETFLAGS] set   0x%02X, result 0x%02X, socket %u\r\n",
                    ioData, telnet_sessions[telnetSocketSel].telnet_flags, telnetSocketSel);
#endif
                ++ioByteCnt;
              }
              else
              {
                telnet_sessions[telnetSocketSel].telnet_flags &= ~ioData;                               // second byte is the RESET mask

                if ((ioData & TFLAG_PURGETXONCONN) && !telnetClientConnected(telnetSocketSel))          // if we just reset the 'PURGETXONCONN' flag and no clients are connected,
                  telnet_sessions[telnetSocketSel].tx_tail = telnet_sessions[telnetSocketSel].tx_head;  // the TX fifo gets purged one last time
#if DEBUG > 0
                consolePrint("[SETTELNETFLAGS] reset 0x%02X, result 0x%02X, socket %u\r\n",
                    ioData, telnet_sessions[telnetSocketSel].telnet_flags, telnetSocketSel);
#endif
                ioOpcode = 0xFF;                                                                        // All done. Set ioOpcode = "No operation"
              }
            }
            else
            {
              consolePrint("[SETTELNETFLAGS] Invalid socked %u\r\n", telnetSocketSel);
              ioOpcode = 0xFF;
            }
          break;

          case  0x45:
            // TELNET TX, SOCKET 0
            //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                             D7 D6 D5 D4 D3 D2 D1 D0    ASCII char to be sent over telnet
            
#if MAX_TELNET_SESSIONS > 0
            if (eth_ok)
              telnet_tx_push(0, ioData);
#endif
          break;

          case  0x46:
            // TELNET TX, SOCKET 1
            //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                             D7 D6 D5 D4 D3 D2 D1 D0    ASCII char to be sent over telnet
            
#if MAX_TELNET_SESSIONS > 1
            if (eth_ok)
            {
              telnet_tx_push(1, ioData);
              if (telnet_sessions[1].telnet_flags & TFLAG_CON_BRIDGE)   // if bridge active on socket 1, data is also forwarded to:
              {
                Serial.printf("%c",ioData);                             // - serial (always)
                if (telnet_sessions[0].telnet_flags & TFLAG_CON_BRIDGE) // - socket 0 if it also has bridge enabled
                  telnet_tx_push(0, ioData);
              }
            }
#endif
          break;

// 0x47 RESERVED for SOCKET 2
// 0x48 RESERVED for SOCKET 3

          case 0x49:
            // GPIOASETRESET
            //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                              X  X  X  X  X  X  X  X    SET mask
            //                              X  X  X  X  X  X  X  X    RESET mask
            if (!ioByteCnt)
            {
              gpioPortA_M |= ioData;                                    // first byte is the SET mask
              ++ioByteCnt;
            }
            else
            {
              gpioPortA_M &= ~ioData;                                   // second byte is the RESET mask
              ioOpcode = 0xFF;                                          // All done. Set ioOpcode = "No operation"

              if (moduleGPIO) 
              {
                Wire.beginTransmission(GPIOEXP_ADDR);
                Wire.write(GPIOA_REG);                                  // Select GPIOA
                Wire.write(gpioPortA_M);                                // Write value
                Wire.endTransmission();
              }
            }
          break;

          case 0x4A:
            // GPIOBSETRESET
            //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                              X  X  X  X  X  X  X  X    SET mask
            //                              X  X  X  X  X  X  X  X    RESET mask
            if (!ioByteCnt)
            {
              gpioPortB_M |= ioData;                                    // first byte is the SET mask
              ++ioByteCnt;
            }
            else
            {
              gpioPortB_M &= ~ioData;                                   // second byte is the RESET mask
              ioOpcode = 0xFF;                                          // All done. Set ioOpcode = "No operation"

              if (moduleGPIO) 
              {
                Wire.beginTransmission(GPIOEXP_ADDR);
                Wire.write(GPIOB_REG);                                  // Select GPIOB
                Wire.write(gpioPortB_M);                                // Write value
                Wire.endTransmission();
              }
            }
          break;

          case 0x4B:
            // SETWATCHDOG
            //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                              M  X  X  X  X  X  X  X    M = 0 = Z80 reset; 1 = Menu restart; d6:d0 = watchdog expire time (*10 s) (0 = disabled)
            //                              X  X  X  X  X  X  X  X    inverted value
            if (!ioByteCnt)
            {
              Z80Watchdog_tmp = ioData;                                 // first byte is the expire time and mode
              Z80WatchDOG_time = 0;
              ++ioByteCnt;
            }
            else
            {
              if (Z80Watchdog_tmp == (uint8_t)~ioData)                  // if value matches,
              {
                Z80WatchDOG_time = Z80Watchdog_tmp;                     // sets the Z80 watchdog expire time (0 = disabled)
                Z80Watchdog_cnt = (Z80WatchDOG_time & 0x7F);
                Z80Watchdog_prescaler = 0;
                Z80watchdog_NOioOper = Z80watchdog_ioOper = false;
#if DEBUG > 0
                consolePrint("\r\n[Z80WDOG] set to %u secs ", Z80Watchdog_cnt*10);
                if (Z80Watchdog_cnt > 0)
                {
                  if (Z80WatchDOG_time & 0x80)
                    consolePrint("(Menu_restart-level Reset)\r\n");
                  else
                    consolePrint("(Z80-level Reset)\r\n");
                }
                else
                  consolePrint("(Disabled)\r\n");
#endif
              }
              else
                consolePrint("\r\n[Z80WDOG] setting error: %u, %u\r\n", Z80Watchdog_tmp, ioData);

              ioOpcode = 0xFF;                                          // All done. Set ioOpcode = "No operation"
            }
          break;

          default:
//#if DEBUG > 0
            consolePrint("\r\n[PROTO] WR DATA (OUT0), 0x%02X with invalid opcode 0x%02X\r\n", ioData, ioOpcode);
//#endif
            ioOpcode = 0xFF;
          break;

        }
        if ((ioOpcode != 0x0A) && (ioOpcode != 0x0C) &&
            (ioOpcode != 0x41) && (ioOpcode != 0x42) && (ioOpcode != 0x44) &&
            (ioOpcode != 0x49) && (ioOpcode != 0x4A) && (ioOpcode != 0x4B))
          ioOpcode = 0xFF;                                              // All done for the single byte Opcodes. 
                                                                        //  Set ioOpcode = "No operation"
      }
      
      // Control bus sequence to exit from a wait state (M I/O write cycle)
      // WAIT_RES is assertet for ~4,5us; Z80 deasserts IORQ in a clock cycle, so no delay is needed (even @0.5MHz it takes 2us, half of the time)
/*
      digitalWrite(BUSREQ_, LOW);                 // Request for a DMA
      digitalWrite(WAIT_RES, HIGH);               // Reset WAIT FF exiting from WAIT state
      digitalWrite(WAIT_RES, LOW);                // Now Z80 is in DMA, so it's safe set WAIT_RES LOW again
      digitalWrite(BUSREQ_, HIGH);                // Resume Z80 from DMA
*/
      BUSREQ_PORT &= ~(1 << BUSREQ_PIN);
      WAIT_RES_PORT |= (1 << WAIT_RES_PIN);
      while (BUSACK_PORTIN & (1 << BUSACK_PIN));
      WAIT_RES_PORT &= ~(1 << WAIT_RES_PIN);
      BUSREQ_PORT |= (1 << BUSREQ_PIN);
    }
//    else if (!digitalRead(RD_))
    else if (!(RD_PORTIN & (1 << RD_PIN)))
    // I/O READ operation requested

    // ----------------------------------------
    // VIRTUAL I/O READ OPERATIONS ENGINE
    // ----------------------------------------
      
    {
      ioAddress = (AD0_PORTIN & (1 << AD0_PIN));                        // this way ioAddress gets the bit weight of the pin, but does not matter
      ioData = 0;                                                       // Clear input data buffer
      if (ioAddress)                                                    // Check the I/O address (only AD0 is checked!)
      // .........................................................................................................
      //
      // AD0 = 1 (I/O read address = 0x01). SERIAL RX.
      //
      // Execute a Serial I/O Read operation.
      // .........................................................................................................
      //
      {
        //
        // SERIAL RX:     
        //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
        //                            ---------------------------------------------------------
        //                             D7 D6 D5 D4 D3 D2 D1 D0    ASCII char read from serial
        //
        // NOTE 1: If there is no input char, a value 0xFF is forced as input char.
        // NOTE 2: The INT_ signal is always reset (set to HIGH) after this I/O operation.
        // NOTE 3: This is the only I/O that do not require any previous STORE Opcode operation (for fast polling).
        // NOTE 4: A "RX buffer empty" flag and a "Last Rx char was empty" flag are available in the SYSFLAG Opcode 
        //         to allow 8 bit I/O.
          
#if DEBUG > 2
        //
        // DEBUG ----------------------------------
          Serial.println();
          Serial.print(F("DEBUG: SER RX"));
          Serial.print(F(" - irqStatus = "));
          Serial.println(irqStatus);
        // DEBUG END ------------------------------
        //
#endif
        ioData = 0xFF;
        if (Serial.available() > 0)
        {
          ioData = Serial.read();
          LastRxIsEmpty = 0;                                            // Reset the "Last Rx char was empty" flag
        }
        // POP dalla RX FIFO telnet socket 0
        else if ((telnet_sessions[0].rx_head != telnet_sessions[0].rx_tail) && (telnet_sessions[0].telnet_flags & TFLAG_CON_BRIDGE))
        {
          ioData = telnet_sessions[0].rx_buf[telnet_sessions[0].rx_tail];
          telnet_sessions[0].rx_tail = (telnet_sessions[0].rx_tail + 1) % TELNET_BUF_SIZE;
          LastRxIsEmpty = 0;
        }
        else LastRxIsEmpty = 1;                                         // Set the "Last Rx char was empty" flag
        digitalWrite(INT_, HIGH);                                       // Reset the INT_ signal (if used)
        irqStatus = irqStatus & B11111110;                              // Reset the serial Rx IRQ status bit (see SYSIRQ Opcode) 
        RxDoneFlag = 1;
      }
      else
      // .........................................................................................................
      //
      // AD0 = 0 (I/O read address = 0x00). EXECUTE READ Opcode.
      //
      // Execute the previously stored I/O read operation with the current data.
      // The code of the I/O operation (Opcode) must be previously stored with a STORE Opcode operation.
      //
      // NOTE: For multi-byte read Opcode (as DATETIME) read sequentially all the data bytes without to send
      //       a STORE Opcode operation before each data byte after the first one.
      // .........................................................................................................
      //
      {
        switch (ioOpcode)
        // Execute the requested I/O READ Opcode. The 0xFF value is reserved as "No operation".
        {
          case  0x80:
            // USER / SELECT KEY:      
            //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                              0  0  0  0  0  0  0  0    no Keys pressed
            //                              0  0  0  0  0  0  0  1    USER Key pressed
            //                              0  0  0  0  0  0  1  0    SELECT Key pressed
            //                              0  0  0  0  0  0  1  1    both Keys pressed
            ioData = buttonsState;
          break;

          case  0x81:
            // GPIOA Read (GPE Option):
            //
            //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                             D7 D6 D5 D4 D3 D2 D1 D0    GPIOA value (see MCP23017 datasheet)
            //
            // NOTE: a value 0x00 is forced if the GPE Option is not present

            if (moduleGPIO) 
            {
              // Set MCP23017 pointer to GPIOA
              Wire.beginTransmission(GPIOEXP_ADDR);
              Wire.write(GPIOA_REG);
              Wire.endTransmission();
              // Read GPIOA
              Wire.beginTransmission(GPIOEXP_ADDR);
              Wire.requestFrom(GPIOEXP_ADDR, 1);
              ioData = Wire.read();
            }
          break;

          case  0x82:
            // GPIOB Read (GPE Option):
            //
            //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                             D7 D6 D5 D4 D3 D2 D1 D0    GPIOB value (see MCP23017 datasheet)
            //
            // NOTE: a value 0x00 is forced if the GPE Option is not present
            
            if (moduleGPIO) 
            {
              // Set MCP23017 pointer to GPIOB
              Wire.beginTransmission(GPIOEXP_ADDR);
              Wire.write(GPIOB_REG);
              Wire.endTransmission();
              // Read GPIOB
              Wire.beginTransmission(GPIOEXP_ADDR);
              Wire.requestFrom(GPIOEXP_ADDR, 1);
              ioData = Wire.read();
            }
          break;

          case  0x83:
            // SYSFLAGS (Various system flags for the OS):
            //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                              X  X  X  X  X  X  X  0    AUTOEXEC not enabled
            //                              X  X  X  X  X  X  X  1    AUTOEXEC enabled
            //                              X  X  X  X  X  X  0  X    DS3231 RTC not found
            //                              X  X  X  X  X  X  1  X    DS3231 RTC found
            //                              X  X  X  X  X  0  X  X    Serial RX buffer empty
            //                              X  X  X  X  X  1  X  X    Serial RX char available
            //                              X  X  X  X  0  X  X  X    Previous RX char valid
            //                              X  X  X  X  1  X  X  X    Previous RX char was a "buffer empty" flag
            //                              X  X  X  0  X  X  X  X    CP/M warm boot message disabled
            //                              X  X  X  1  X  X  X  X    CP/M warm boot message enabled              
            //
            // NOTE 1: Currently only D0-D4 are used
            // NOTE 2: The D4 flag is set/reset using the D0 bit of the SETOPT Opcode (see SETOPT for more info)

            uint8_t seravail;
            seravail = 0;
            if (InputAvailable())                                       // InputAvailable() takes into account the 'TFLAG_CON_BRIDGE' flag
              seravail = 4;
            ioData = autoexecFlag | (rtcData.foundRTC << 1) | seravail | ((LastRxIsEmpty > 0) << 3) 
                     | (cpmWarmBootFlg << 4);
          break;

          case  0x84:
            // DATETIME (Read date/time and temperature from the RTC. Binary values): 
            //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                I/O DATA 0   D7 D6 D5 D4 D3 D2 D1 D0    seconds [0..59]     (1st data byte)
            //                I/O DATA 1   D7 D6 D5 D4 D3 D2 D1 D0    minutes [0..59]
            //                I/O DATA 2   D7 D6 D5 D4 D3 D2 D1 D0    hours   [0..23]
            //                I/O DATA 3   D7 D6 D5 D4 D3 D2 D1 D0    day     [1..31]
            //                I/O DATA 4   D7 D6 D5 D4 D3 D2 D1 D0    month   [1..12]
            //                I/O DATA 5   D7 D6 D5 D4 D3 D2 D1 D0    year    [0..99]
            //                I/O DATA 6   D7 D6 D5 D4 D3 D2 D1 D0    tempC   [-128..127] (7th data byte)
            //
            // NOTE 1: If RTC is not found all read values wil be = 0
            // NOTE 2: Overread data (more then 7 bytes read) will be = 0
            // NOTE 3: The temperature (Celsius) is a byte with two complement binary format [-128..127]

            if (rtcData.foundRTC)
            {
              if (ioByteCnt == 0) readRTC(&rtcData);                    // Read from RTC
              if (ioByteCnt < 7)
              // Send date/time (binary values) to Z80 bus
              {
                switch (ioByteCnt)
                {
                  case 0: ioData = rtcData.seconds; break;
                  case 1: ioData = rtcData.minutes; break;
                  case 2: ioData = rtcData.hours; break;
                  case 3: ioData = rtcData.day; break;
                  case 4: ioData = rtcData.month; break;
                  case 5: ioData = rtcData.year; break;
                  case 6: ioData = rtcData.tempC; break;
                }
                ioByteCnt++;
                if (ioByteCnt >= 7)
                  ioOpcode = 0xFF;                                      // All done. Set ioOpcode = "No operation"
              }
            }
            else ioOpcode = 0xFF;                                       // Nothing to do. Set ioOpcode = "No operation"
          break;

          case  0x85:
            // DISK EMULATION
            // ERRDISK - read the error code after a SELDISK, SELSECT, SELTRACK, WRITESECT, READSECT 
            //           or SDMOUNT operation
            //
            //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                             D7 D6 D5 D4 D3 D2 D1 D0    DISK error code (binary)
            //
            //
            // Error codes table:
            //
            //    error code    | description
            // ---------------------------------------------------------------------------------------------------
            //        0         |  No error
            //        1         |  DISK_ERR: the function failed due to a hard error in the disk function, 
            //                  |   a wrong FAT structure or an internal error
            //        2         |  NOT_READY: the storage device could not be initialized due to a hard error or 
            //                  |   no medium
            //        3         |  NO_FILE: could not find the file
            //        4         |  NOT_OPENED: the file has not been opened
            //        5         |  NOT_ENABLED: the volume has not been mounted
            //        6         |  NO_FILESYSTEM: there is no valid FAT partition on the drive
            //       16         |  Illegal disk number
            //       17         |  Illegal track number
            //       18         |  Illegal sector number
            //       19         |  Reached an unexpected EOF
            //
            //
            //
            //
            // NOTE 1: ERRDISK code is referred to the previous SELDISK, SELSECT, SELTRACK, WRITESECT or READSECT
            //         operation
            // NOTE 2: Error codes from 0 to 6 come from the PetitFS library implementation
            // NOTE 3: ERRDISK must not be used to read the resulting error code after a SDMOUNT operation 
            //         (see the SDMOUNT Opcode)

            ioData = diskErr;
          break;

          case  0x86:
            // DISK EMULATION
            // READSECT - read 512 data bytes sequentially from the current emulated disk/track/sector:
            //
            //                 I/O DATA:   D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                 I/O DATA 0  D7 D6 D5 D4 D3 D2 D1 D0    First Data byte
            //
            //                      |               |
            //                      |               |
            //                      |               |                 <510 Data Bytes>
            //                      |               |
            //
            //               I/O DATA 127  D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                             D7 D6 D5 D4 D3 D2 D1 D0    512th Data byte (Last byte)
            //
            //
            // Reads the current sector (512 bytes) of the current track/sector, one data byte each call. 
            // All the 512 calls must be always performed sequentially to have a READSECT operation correctly done. 
            // If an error occurs during the READSECT operation, all subsequent read data will be = 0.
            // If an error occurs calling any DISK EMULATION Opcode (SDMOUNT excluded) immediately before the READSECT 
            //  Opcode call, all the read data will be will be = 0 and the READSECT operation will not be performed.
            // Errors are stored into "diskErr" (see ERRDISK Opcode).
            //
            // NOTE 1: Before a READSECT operation at least a SELTRACK or a SELSECT must be always performed
            // NOTE 2: Remember to open the right "disk file" at first using the SELDISK Opcode

            if (!ioByteCnt)
            // First byte of 512, so set the right file pointer to the current emulated track/sector first
            {
#if DEBUG > 2
              Serial.println(F("[READSEC] start"));
#endif
              if ((trackSel < 512) && (sectSel < 32) && (!diskErr))
              // Sector and track numbers valid and no previous error; set the LBA-like logical sector
              {
                diskErr = seekSD((trackSel << 5) | sectSel);    // Set the starting point inside the "disk file"
                                                                //  generating a 14 bit "disk file" LBA-like 
                                                                //  logical sector address created as TTTTTTTTTSSSSS
#if DEBUG > 1
                if (diskErr)
                  Serial.printf("[READSECT] seekSD error %u\r\n", diskErr);
#endif
              }
            }
            if (!diskErr)
            // No previous error (e.g. selecting disk, track or sector)
            {
              byte tempByte = ioByteCnt % 32;                   // [0..31]
              if (!tempByte)
              // Read 32 bytes of the current sector on SD in the buffer (every 32 calls, starting with the first)
              {
                diskErr = readSD(bufferSD, &numReadBytes); 
                if (numReadBytes < 32) diskErr = 19;            // Reached an unexpected EOF
#if DEBUG > 1
                if (diskErr)
                  Serial.printf("[READSECT] readSD error %u\r\n", diskErr);
#endif
              }
              if (!diskErr) ioData = bufferSD[tempByte];        // If no errors, exchange current data byte with the CPU
            }
            if (ioByteCnt >= 511) 
            {
#if DEBUG > 2
              Serial.println(F("[READSEC] end"));
#endif
              ioOpcode = 0xFF;                                  // All done. Set ioOpcode = "No operation"
            }
            ioByteCnt++;                                        // Increment the counter of the exchanged data bytes
          break;

          case  0x87:
            // DISK EMULATION
            // SDMOUNT - mount a volume on SD, returning an error code (binary):
            //
            //                 I/O DATA 0: D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                             D7 D6 D5 D4 D3 D2 D1 D0    error code (binary)
            //
            //
            //
            // NOTE 1: This Opcode is "normally" not used. Only needed if using a virtual disk from a custom program
            //         loaded with iLoad or with the Autoboot mode (e.g. ViDiT). Can be used to handle SD hot-swapping
            // NOTE 2: For error codes explanation see ERRDISK Opcode
            // NOTE 3: Only for this disk Opcode, the resulting error is read as a data byte without using the 
            //         ERRDISK Opcode

            ioData = mountSD(&filesysSD);
          break;

          case  0x88:
            // ATXBUFF - return the current available free space (in bytes) in the TX buffer:
            //
            //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                             D7 D6 D5 D4 D3 D2 D1 D0    free space in bytes (binary)
            //
            // NOTE: This Opcode is intended to avoid delays in serial Tx operations, as the IOS holds the Z80
            //       in a wait status if the TX buffer is full. This is no good in multitasking enviroments.

            ioData = Serial.availableForWrite() ;
          break;

          case  0x89:
            // SYSIRQ - return the "interrupt status byte":
            //
            //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                              X  X  X  X  X  X  X  0    Serial Rx IRQ not set
            //                              X  X  X  X  X  X  X  1    Serial Rx IRQ set
            //                              X  X  X  X  X  X  0  X    Systick IRQ not set
            //                              X  X  X  X  X  X  1  X    Systick IRQ set
            //
            //
            // The INT_ signal is shared among various interrupt requests. This allows to use the simplified 
            //  Mode 1 scheme of the Z80 CPU (fixed jump to 0x0038 on INT_ signal active) with multiple interrupt 
            //  causes.
            // The SYSIRQ purpose is to allow the Z80 CPU to know the exact causes of the occurred interrupts 
            //  reading the "interrupt status byte" that contains up to eight "interrupt status bits". 
            // So the ISR (Interrupt Service Routine) should be structured to read at first the 
            //  "interrupt status byte" using the SYSIRQ Opcode, and than execute the needed actions before 
            //  return to the normal execution.
            // Note that multiple causes/bits could be active.
            // 
            //
            //
            // NOTE 1: Only D0 and D1 "interrupt status bit" are currently used.
            // NOTE 2: After the SYSIRQ call all the "interrupt status bits" are cleared.
            // NOTE 3: If more than one IRQ trigger is enabled, you have to call SYSIRQ always from inside the 
            //         ISR (on the Z80 side) to know the triggered IRQs.
             
            ioData = irqStatus;
            irqStatus = 0;                        // Reset all the "interrupt status bits"

            //
            // DEBUG ----------------------------------
#if DEBUG > 2
              Serial.println();
              Serial.print(F("DEBUG: SYSIRQ"));
              Serial.print(F(" - ioData = "));
              Serial.println(ioData);
#endif
// DEBUG END ------------------------------
            //
        
          break;

          case  0x8A:
            // SPP EMULATION
            // GETSPP - read the Status Lines of the SPP Port and the SPP emulation status:
            //
            //                  I/O DATA:  D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                              0  0  0  0  0  0  0  0    SPP emulation disabled
            //                             D7 D6 D5 D4 D3  0  0  1    SPP emulation enabled
            //
            //                  bit  | SPP Status line 
            //                  ----------------------------------
            //                   D0  | 1 (SPP emulation enabled) 
            //                   D1  | 0 (not used)
            //                   D2  | 0 (not used)
            //                   D3  | ACK (active Low)
            //                   D4  | BUSY (active High)
            //                   D5  | PAPEREND (active High)
            //                   D6  | SELECT (active High)
            //                   D7  | ERROR (active Low)
            //
            // If the SPP mode is enabled read the SPP Status Lines.
            // If the SPP mode is disabled (or the GPE is not installed) a byte of all 0s will be retrivied.
            //
            // NOTE: to use GETSPP the SETSPP Opcode should be called first to activate the SPP mode of the GPIO port.

            if (SPPmode)
            {
              // Set MCP23017 pointer to GPIOA
              Wire.beginTransmission(GPIOEXP_ADDR);
              Wire.write(GPIOA_REG);
              Wire.endTransmission();

              // Read GPIOA (SPP Status Lines)
              Wire.beginTransmission(GPIOEXP_ADDR);
              Wire.requestFrom(GPIOEXP_ADDR, 1);
              ioData = Wire.read();
              ioData = (ioData & 0b11111000) | 0b00000001;      // Set D0 = 1, D1 = D2 = 0
            }
          break;

          case 0xC0:
            // GETTELNETFLAGS
            if (telnetSocketSel < MAX_TELNET_SESSIONS)
            {
              ioData = telnet_sessions[telnetSocketSel].telnet_flags;
#if DEBUG > 0
              consolePrint("[GETTELNETFLAGS] %u, socket %u\r\n", telnet_sessions[telnetSocketSel].telnet_flags, telnetSocketSel);
#endif
            }
            else
            {
              Serial.printf("[GETTELNETFLAGS] Invalid socked %u\r\n", telnetSocketSel);
              ioData = 0;
            }
          break;

          case 0xC1:
            // GETBANK
            ioData = currentBank;
#if DEBUG > 1
            consolePrint("[GETBANK] %u\r\n", currentBank);
#endif
          break;

          case 0xC2:
            // GETTELNETSTAT03 (Various telnet-related status flags) SESSION 0...3
            //
            //                  I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                              ---------------------------------------------------------
            //                                X  X  X  X  X  X  X  0    No chars available    session 0
            //                                X  X  X  X  X  X  X  1    Chars available       session 0
            //                                X  X  X  X  X  X  0  X    No clients connected  session 0
            //                                X  X  X  X  X  X  1  X    A client is connected session 0
            //                                X  X  X  X  X  0  X  X    No chars available    session 1
            //                                X  X  X  X  X  1  X  X    Chars available       session 1
            //                                X  X  X  X  0  X  X  X    No clients connected  session 1
            //                                X  X  X  X  1  X  X  X    A client is connected session 1
            //                                [...]
            //                                1  X  X  X  0  X  X  X    A client is connected session 3
            //
            ioData = 0;
            for (uint8_t i = MAX_TELNET_SESSIONS; i > 0; --i)
            {
              ioData <<= 2;
              ioData |= ((uint8_t)((telnet_sessions[i-1].rx_head != telnet_sessions[i-1].rx_tail) | (uint8_t)(telnetClientConnected(i-1) << 1)));
            }
          break;

          case 0xC3:
            // TELNET RX, SOCKET 0
            //
            //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                             D7 D6 D5 D4 D3 D2 D1 D0    ASCII char read from telnet
            //
            // NOTE: If there is no input char, a value 0xFF is forced as input char.
            // NOTE: this function is not available when flag 'TFLAG_CON_BRIDGE' is active (SOCKET 0 bridged to serial console)
            //
            ioData = 0xFF;
#if MAX_TELNET_SESSIONS > 0
            if (telnet_sessions[0].rx_head != telnet_sessions[0].rx_tail && !(telnet_sessions[0].telnet_flags & TFLAG_CON_BRIDGE))
            {
              ioData = telnet_sessions[0].rx_buf[telnet_sessions[0].rx_tail];
              telnet_sessions[0].rx_tail = (telnet_sessions[0].rx_tail + 1) % TELNET_BUF_SIZE;
            }
#endif
          break;

          case 0xC4:
            // TELNET RX, SOCKET 1
            //
            //                I/O DATA:    D7 D6 D5 D4 D3 D2 D1 D0
            //                            ---------------------------------------------------------
            //                             D7 D6 D5 D4 D3 D2 D1 D0    ASCII char read from telnet
            //
            // NOTE: If there is no input char, a value 0xFF is forced as input char.
            //
            ioData = 0xFF;
#if MAX_TELNET_SESSIONS > 1
            if (telnet_sessions[1].rx_head != telnet_sessions[1].rx_tail)
            {
              ioData = telnet_sessions[1].rx_buf[telnet_sessions[1].rx_tail];
              telnet_sessions[1].rx_tail = (telnet_sessions[1].rx_tail + 1) % TELNET_BUF_SIZE;

              if (telnet_sessions[1].telnet_flags & TFLAG_CON_BRIDGE)   // if bridge active on socket 1, data is also forwarded to:
              {
                Serial.printf("%c",ioData);                             // - serial (always)
                if (telnet_sessions[0].telnet_flags & TFLAG_CON_BRIDGE) // - socket 0 if it also has bridge enabled
                  telnet_tx_push(0, ioData);
              }
            }
//            Serial.printf("%c",ioData); //@@@
#endif
          break;

// 0xC5 RESERVED for SOCKET 2
// 0xC6 RESERVED for SOCKET 3

          case  0xC7:
            // ETH_REMOTE_IP - read 4 data bytes from the REMOTE IP register + 1 byte proxy state of currently selected socket
            //
            //                Bytes 0 - 3 = IP address of connected client
            //                Byte 4      = proxy state: 0 = IP is not proxied; 1 = parsing proxy string in progress (invalid data in IP field); 2 = IP is proxied
            //
            if (ioByteCnt < 4)
              ioData = telnet_sessions[telnetSocketSel].remoteIP[ioByteCnt];
            else
            {
              ioData = telnet_sessions[telnetSocketSel].proxy_state;
              ioOpcode = 0xFF;                                  // All done. Set ioOpcode = "No operation"
            }

            ioByteCnt++;                                        // Increment the counter of the exchanged data bytes
          break;


          default:
//#if DEBUG > 0
            Serial.printf("\r\n[PROTO] RD DATA (IN0) with invalid opcode 0x%02X, \r\n", ioOpcode);
//#endif
            ioOpcode = 0xFF;
          break;
        }
        if ((ioOpcode != 0x84) && (ioOpcode != 0x86) && (ioOpcode != 0xC7)) ioOpcode = 0xFF;  // All done for the single byte Opcodes. 
                                                                                              //  Set ioOpcode = "No operation"
      }
      DDRA = 0xFF;                                // Configure Z80 data bus D0-D7 (PA0-PA7) as output
      PORTA = ioData;                             // Current output on data bus
/*
      // Control bus sequence to exit from a wait state (M I/O read cycle), modified for Z80_MBC2-V (WAIT_RES)
      digitalWrite(BUSREQ_, LOW);                 // Request for a DMA
      digitalWrite(WAIT_RES, HIGH);               // Now is safe reset WAIT FF (exiting from WAIT state)
      delayMicroseconds(BUS_DELAY_US);            // Wait (2us ?) just to be sure that Z80 read the data and go HiZ
      DDRA = 0x00;                                // Configure Z80 data bus D0-D7 (PA0-PA7) as input with pull-up
      PORTA = 0xFF;
      digitalWrite(WAIT_RES, LOW);                // Now Z80 is in DMA (HiZ), so it's safe set WAIT_RES_ LOW again
      digitalWrite(BUSREQ_, HIGH);                // Resume Z80 from DMA
*/
      BUSREQ_PORT &= ~(1 << BUSREQ_PIN);
      WAIT_RES_PORT |= (1 << WAIT_RES_PIN);
      while (BUSACK_PORTIN & (1 << BUSACK_PIN));
      DDRA = 0x00;                                // Configure Z80 data bus D0-D7 (PA0-PA7) as input with pull-up
      PORTA = 0xFF;
      WAIT_RES_PORT &= ~(1 << WAIT_RES_PIN);
      BUSREQ_PORT |= (1 << BUSREQ_PIN);
    }
    else
    // INTERRUPT operation setting IORQ_ LOW

    // ----------------------------------------
    // VIRTUAL INTERRUPT
    // ----------------------------------------

    {
//      digitalWrite(INT_, HIGH);                 // Reset the INT_ signal
      INT_PORT |= (1 << INT_PIN);

#if DEBUG > 2
      //
      // DEBUG ----------------------------------
        Serial.println();
        Serial.print(F("DEBUG: INT ACK cycle"));
        Serial.print(F(" - irqStatus = "));
        Serial.println(irqStatus);
      // DEBUG END ------------------------------
      //
#endif        
      // Control bus sequence to exit from a wait state (M interrupt cycle)
      BUSREQ_PORT &= ~(1 << BUSREQ_PIN);          // BUSREQ low (request for a DMA)
      WAIT_RES_PORT |= (1 << WAIT_RES_PIN);       // WAIT_RES high (Reset WAIT FF exiting from WAIT state)
      while (BUSACK_PORTIN & (1 << BUSACK_PIN));  // wait for BUSACK
      WAIT_RES_PORT &= ~(1 << WAIT_RES_PIN);      // WAIT_RES low (Now Z80 is in DMA, so it's safe set WAIT_RES_ LOW again)
      BUSREQ_PORT |= (1 << BUSREQ_PIN);           // BUSREQ high
    }
  }
  else
    Z80watchdog_NOioOper = true;

#if DEBUG > 3
  if (ioOpcode == 0xFF)
    digitalWrite(SEL_WDOG, LOW);                  // debug trigger
#endif

//
// TIMER1 system tick 10ms
//
  if (TIFR1 & (1 << OCF1A))
  {
    TIFR1 = (1 << OCF1A);                         // Clear flag
    ++tick100ms;

//
// Events run every 10ms
//

  if (eth_ok)
    ethTick = 1;                                  // calls 'telnet_handler() at least every 10ms


    if (tick100ms >= 10)
    {
      tick100ms = 0;
      ++tick1s;

//
// Events run every 100ms
//

      // Reads buttons and feeds hardware watchdog
      byte tempByte = (USER_PORT & (1 << USER_PIN));          // Save USER led status (note: tempByte will get the pin weight instead of 1)
      USER_DDR &= ~(1 << USER_PIN);                           // DDR = 0 → input
      USER_PORT |=  (1 << USER_PIN);                          // PORT = 1 → pull-up

#if DEBUG < 4
      byte tempByte2 = (SEL_WDOG_PORT & (1 << SEL_WDOG_PIN));
      SEL_WDOG_DDR  &= ~(1 << SEL_WDOG_PIN);
      SEL_WDOG_PORT |=  (1 << SEL_WDOG_PIN);
#endif
      delayMicroseconds(2);

      buttonsState = (USER_PORTIN & (1 << USER_PIN)) ? 0 : 1; // 1 if level low (button pressed)
      if (tempByte)
        USER_PORT |=  (1 << USER_PIN);                        // restore previous level
      else
        USER_PORT &= ~(1 << USER_PIN);

      USER_DDR |= (1 << USER_PIN);                            // back to OUTPUT

#if DEBUG < 4
      if (!(SEL_WDOG_PORTIN & (1 << SEL_WDOG_PIN)))
        buttonsState |= 2;

      if (tempByte2)
        SEL_WDOG_PORT &=  ~(1 << SEL_WDOG_PIN);               // restore inverted state (watchdog feed)
      else
        SEL_WDOG_PORT |= (1 << SEL_WDOG_PIN);

      SEL_WDOG_DDR |= (1 << SEL_WDOG_PIN);                    // SELECT back to OUTPUT
#endif

      if (buttonsState)
        sh1106_contrast(SH1106_DEFAULT_CONTRAST);

//
// telnet special commands
//
      if (telnetSpecialCmds == CMD_MAIN_MENU)
      {
        sysMenu(1);
      }
      else if (telnetSpecialCmds == CMD_Z80_RESET)
      {
        Z80_async_reset();
        consolePrint("_Z80_res_\r\n");
      }
      else if (telnetSpecialCmds == CMD_ENABLE_TICK_INT)
      {
        Z80IntSysTick = 1;
        consolePrint("_Ien_\r\n");
      }
      else if (telnetSpecialCmds == CMD_DISABLE_TICK_INT)
      {
        Z80IntSysTick = 0;
        consolePrint("_Idis_\r\n");
      }
      telnetSpecialCmds = 0;


      if (tick1s >= 10)
      {
        tick1s = 0;

//
// Events run every second
//

        sh1106_1s();

//
// Z80 watchdog
//
        if (Z80WatchDOG_time > 0)
        {
          if (Z80watchdog_NOioOper && Z80watchdog_ioOper)         // if a non-stale I/O operation occurred,
          {
            Z80Watchdog_prescaler = 0;
            Z80Watchdog_cnt = (Z80WatchDOG_time & 0x7F);          // feeds the watchdog
            Z80watchdog_NOioOper = Z80watchdog_ioOper = false;
          }
          else
          {
            ++Z80Watchdog_prescaler;
            if (Z80Watchdog_prescaler >= 10)                      // 10 secs prescaler
            {
              Z80Watchdog_prescaler = 0;

              --Z80Watchdog_cnt;
              if (Z80Watchdog_cnt == 0)                           // time expired:
              {
                ++Z80wdog_counters.count;                         // increments Z80 watchdog events counter

                readRTC(&rtcData);
                Z80wdog_counters.seconds = rtcData.seconds;
                Z80wdog_counters.minutes = rtcData.minutes;
                Z80wdog_counters.hours = rtcData.hours;
                Z80wdog_counters.day = rtcData.day;
                Z80wdog_counters.month = rtcData.month;
                Z80wdog_counters.year = rtcData.year;

                EEPROM.put(EE_Z80WATCHDOG_ADDR, Z80wdog_counters);

                if (Z80WatchDOG_time & 0x80)                      // if D7 of watchdog setting is SET,
                  sysMenu(0);                                     // restarts the menu
                else
                  Z80_async_reset();                              // otherwise, Z80 reset

                Z80WatchDOG_time = 0;                             // watchdog is now disabled (must be re-enabled by Z80)
              }
            }
          }
        }
      } // tick 1 sec
    } // tick 100ms

    if (Z80IntSysTick)
    // Systick interrupt generation is enabled. Check if the INT_ signal must be activated
    {
      sysTickCnt += 10;
      if (sysTickCnt >= sysTickTime)
      // <sysTickTime> milliseconds are elapsed, so a Systick interrupt is required
      {
        sysTickCnt = 0;
        irqPending = 1;                           // INT should be asserted on a bank-safe basis: set a flag instead of driving the signal
        irqStatus = irqStatus | B00000010;        // Set the Systick IRQ status bit (see SYSIRQ Opcode)
      }
    }
  }   // tick 10ms

  // serial / telnet event interrupt
  if (InputAvailable() && (Z80IntRx == 1) && (RxDoneFlag))
  {
    irqPending = 1;                               // INT should be asserted on a bank-safe basis
    irqStatus = irqStatus | B00000001;            // Set the serial Rx IRQ status bit (see SYSIRQ Opcode)
    RxDoneFlag = 0;
  }
//
// Considerazioni asserzione INT:
//
// L'obiettivo di questo meccanismo e' evitare che un INT venga asserito interrompendo una sequenza I/O Z80, che deve essere atomica.
// Ci sono due punti di intervento: evitare asserzione fra l'esecuzione di un I/O opcode e quello dei dati; evitare asserzione durante I/O stesso.

// 1) 'POLL_WAIT_BEFORE_INT' = 1
// Se lo Z80 inizia un ciclo I/O (che innesca WAIT FF) proprio nel momento in cui noi asseriamo INT, e se il firmware Z80 non lo ignora (come invece dovrebbe),
// l'INT (che viene servito DOPO l'operazione I/O) puo interporsi fra due operazioni I/O (scrittura ioOpcode / scrittura dati) col rischio di corromperla
// se la ISR esegue altri I/O.
// Per evitarlo, asseriamo BUSREQ, attendiamo BUSACK o WAIT e poi polliamo WAIT: se alto, il processore e' stato fermato lontano da una operazione I/O
// e quindi possiamo asserire INT e poi revocare BUSREQ.
// Se 'POLL_WAIT_BEFORE_INT' e' 0, questa precauzione non e' attiva e l'INT viene asserito a prescindere dallo stato del WAIT (funzionamento originale).
//
// 2) 'a' = 1
// Per evitare di assere INT nel mezzo di una coppia di I/O (ioOpcode / dati), testiamo 'ioOpcode': se non e' 0xFF, l'INT non viene asserito.
// Questo espone al rischio che una sequenza multibyte volutamente non completata (es. lettura RTC 0x84 senza leggere il byte della temperatura)
// prevenga la generazione di tutti gli INT fino a quando, eventualmente, lo Z80 non scrivera' (e completera') un diverso opcode.
//
// Se la define 'NO_INT_ON_ANY_OPCODE' e' 0, il meccanismo e' attivo per il solo opcode 'SETBANK', che richiede un discorso a parte.
// Asserire un interrupt quando il banco di RAM selezionato non contiene la ISR ha risultati sicuramente catastrofici.
// La nostra variabile 'currentBank' contiene il n. del banco attualmente attivo e la 'irqSafeBank' indica quello INT-safe (o tutti, se == 0xFF).
// Se ioOpcode e' == 0x0D (SETBANK), non sappiamo quale banco sta per essere selezionato e quindi NON SI DEVE ASSOLUTAMENTE asserire INT.
// Dopo il suo completamento (quindi con ioOpcode == 0xFF) il nuovo banco selezionato sara' noto in 'currentBank' e potra' essere valutata
// l'opportunita' di asserire l'INT o no (se il banco non e' int-safe).
//

#if NO_INT_ON_ANY_OPCODE == 0
  if (irqPending && ioOpcode != 0x0D && (currentBank == irqSafeBank || irqSafeBank == 0xFF))          // No INT during SETBANK only
#else
  if (irqPending && ioOpcode == 0xFF && (currentBank == irqSafeBank || irqSafeBank == 0xFF))          // No INT during any opcode
#endif
  {
#if POLL_WAIT_BEFORE_INT == 0
    INT_PORT &= ~(1 << INT_PIN);                  // INT asserted with no WAIT check
    irqPending = 0;
#else
    BUSREQ_PORT &= ~(1 << BUSREQ_PIN);            // BUSREQ LOW
    while ((BUSACK_PORTIN & (1 << BUSACK_PIN)) && // waits for BUSACK low (compatibility with any Z80 clock speed)
        (WAIT_PORTIN & (1 << WAIT_PIN)));         // also abort if WAIT low
    if (WAIT_PORTIN & (1 << WAIT_PIN))            // if WAIT is high, we are safe to assert INT
    {
      INT_PORT &= ~(1 << INT_PIN);                // with Z80 @8MHz, the cycle may take ~0.8us - 1,4us depending on the BUSACK timing.
      irqPending = 0;                             // INT is asserted ~500-800ns after BUSACK goes low
    }
    BUSREQ_PORT |= (1 << BUSREQ_PIN);
#endif
  }

//
// OLED Display Z80 commands (clear, set cursor position, char write).
//
// A single character is written at a time, to minimize blocking and also because Z80 can issue a single char write command per loop.
// An I/O operation will certainly take place shortly after the LCD write opcode has been activated, so this must be as quick as possible.
// This is tipically a I/O RD operation for serial bytes.
//
  if (disp_ok)
  {
    switch (dispOp)
    {
      case DISPOP_CLEAR:
        sh1106_clear();
      break;
      
      case DISPOP_SETPOS:
        sh1106_set_pos(disp_X, disp_Y);
      break;

      case DISPOP_WRCHAR:
        sh1106_write_char(dispData);
      break;

      case DISPOP_SETCONTRAST:
        sh1106_contrast(dispData);
      break;
    }
  }
  dispOp = DISPOP_IDLE;


//
// Calls ethernet handler on 10ms tick (ethTick) or if more than 100 characters
// are in the FIFO TX buffer and a client is connected
//
  if (eth_ok)
  {
    uint16_t fifoAvail = 0;
    for (uint8_t i = 0; i < MAX_TELNET_SESSIONS; ++i)
    {
      if (telnetClientConnected(i))
        fifoAvail += (telnet_sessions[i].tx_head - telnet_sessions[i].tx_tail + TELNET_BUF_SIZE) % TELNET_BUF_SIZE;
    }
    if (ethTick != 0 || fifoAvail > 100)
    {
      ethTick = 0;
      telnet_handler();
    }
  }
}


// ------------------------------------------------------------------------------

// Generic routines

// ------------------------------------------------------------------------------

void Z80_async_reset(void)
{
  digitalWrite(RESET_, LOW);
  digitalWrite(WAIT_RES, HIGH);
  delay(100);
  digitalWrite(WAIT_RES, LOW);
  digitalWrite(RESET_, HIGH);
}


void LoadNetworkConfig(void)
{
  EEPROM.get(EEPROM_NETCFG_ADDR, netcfg);

  if (netcfg.magic != NETCFG_MAGIC)
  {
    Serial.println(F("Loading default network settings"));
    uint8_t defaultMac[6] = { 0x00, 0x12, 0x34, 0x64, 0x67, 0x90 };

    memcpy(netcfg.mac, defaultMac, 6);
    netcfg.ip[0] = 192; netcfg.ip[1] = 168; netcfg.ip[2] = 0; netcfg.ip[3] = 60;
    netcfg.netmask[0] = 255; netcfg.netmask[1] = 255; netcfg.netmask[2] = 255; netcfg.netmask[3] = 0;
    netcfg.gateway[0] = 192; netcfg.gateway[1] = 168; netcfg.gateway[2] = 0; netcfg.gateway[3] = 1;
    netcfg.trusted_proxy[0] = 0; netcfg.trusted_proxy[1] = 0; netcfg.trusted_proxy[2] = 0; netcfg.trusted_proxy[3] = 0;
    netcfg.dns1[0] = 8; netcfg.dns1[1] = 8; netcfg.dns1[2] = 8; netcfg.dns1[3] = 8;
    netcfg.dns2[0] = 1; netcfg.dns2[1] = 1; netcfg.dns2[2] = 1; netcfg.dns2[3] = 1;
    netcfg.tcpTimeout = 2000;
    netcfg.retries = 8;
    strcpy(netcfg.sock0_password, SOCKET0PWD_DEF);
    netcfg.magic = NETCFG_MAGIC;

    EEPROM.put(EEPROM_NETCFG_ADDR, netcfg);
  }

  if (strlen(netcfg.sock0_password) > NETCFG_SOCK0_PWDLEN)                                            // if missing for any reason,
    netcfg.sock0_password[NETCFG_SOCK0_PWDLEN] = '\0';                                                // adds the trailing NULL (array size is NETCFG_SOCK0_PWDLEN+1)
}


/*
void printBinaryByte(byte value)
{
  for (byte mask = 0x80; mask; mask >>= 1)
  {
    Serial.print((mask & value) ? '1' : '0');
  }
}
*/

// ------------------------------------------------------------------------------

void printMsg1(void)
{
  consolePrint("\r\nPress CR to accept, ESC to exit or any other key to change\r\n");
}

// ------------------------------------------------------------------------------

byte WaitAndBlink(baudRecCheck baudRecSwitch)
// Wait for a char from the serial port while IOS led blinks and do the Baud Recovery if requested.
// If <baudRecSwitch> = CHECK the User key is checked if it remains pressed at least 3 seconds ("long User key pressed" 
// event).
// In this happens the Baud Recovery procedure is done if the current serial speed is different from the default 
// value (115200).
// When the Baud Recovery procedure is executed the serial port speed is set at the default value (115200) and both
// the USER and IOS leds blink quickly. The default serial port speed will be effective after the next reset.
{
  byte                  UserKeyLongPressed = 1;       // Flag for the "User key long pressed" event
  static unsigned long  timeStamp1;                   // Timestamps
  unsigned long         timeStamp2;
  byte                  keypressed = 0xFF;
  byte                  dispOff = 255;

  timeStamp2 = millis();
  while (!InputAvailable())
  {
    if (eth_ok)
    {
      telnet_handler();                               // l'handler telnet deve girare a prescindere dallo stato del bridging seriale
    }

    if ((millis() - timeStamp2) < 3000)
    // Check is User key remains pressed for 3s more
    {
      pinMode(USER, INPUT_PULLUP);                    // Set read mode for USER key
      if (digitalRead(USER)) UserKeyLongPressed = 0;  // Clear the flag if USER key was released at least once before 3s
      pinMode(USER, OUTPUT);                          // Set write mode for USER led
      delay(10);                                      // Just to dimm USER led up to fade off
    }
    if ((millis() - timeStamp1) > 300)
    // Blink IOS led
    {
      digitalWrite(LED_IOS,!digitalRead(LED_IOS));
      timeStamp1 = millis();

      if (dispOff > 0 && disp_ok)
      {
        --dispOff;
        if (dispOff == 0)
          sh1106_contrast(0);
      }
    }
    if (((millis() - timeStamp2) > 3000) && (UserKeyLongPressed) && (baudRecSwitch = CHECK) && (EEPROM.read(EE_SERBAUD_ADDR) != 9))
    // Do the Baud Recovery procedure.
    // The User key was pressed for at least 3s and the serial port speed is different from default value (115200),
    // so set the default baud rate (115200) for the serial port and wait for a reboot while both IOS and USER leds 
    // blink quickly.
    {
      digitalWrite(LED_IOS,LOW);
      EEPROM.update(EE_SERBAUD_ADDR, 9);              // Set the default baud rate (115200)
      consolePrint("\r\n\nIOS: Baud recovery done - Please reboot now!\r\n");
      while(1)
      {
        if (eth_ok)
        {
          telnet_handler();
        }

        if ((millis() - timeStamp1) > 100)
        {
          digitalWrite(LED_IOS,!digitalRead(LED_IOS));
          digitalWrite(USER,!digitalRead(USER));
          timeStamp1 = millis();
        }
      }
    }
  }
  if (Serial.available())
    keypressed = Serial.read();
  else if (telnet_sessions[0].telnet_flags & TFLAG_CON_BRIDGE)
  {
    keypressed = telnet_sessions[0].rx_buf[telnet_sessions[0].rx_tail];
    telnet_sessions[0].rx_tail = (telnet_sessions[0].rx_tail + 1) % TELNET_BUF_SIZE;
  }
  return(keypressed);
}


//
// Returns TRUE if a char is available on serial line OR (available on telnet line AND bridge is active)
// When 'TFLAG_CON_BRIDGE' is set, the telnet stream acts as the serial stream from IOS point of view
//
bool InputAvailable()
{
    return Serial.available() ||
        ((telnet_sessions[0].rx_head != telnet_sessions[0].rx_tail) && (telnet_sessions[0].telnet_flags & TFLAG_CON_BRIDGE));
}

// ------------------------------------------------------------------------------

// RTC Module routines

// ------------------------------------------------------------------------------


byte decToBcd(byte val)
// Convert a binary byte to a two digits BCD byte
{
  return( (val/10*16) + (val%10) );
}

// ------------------------------------------------------------------------------

byte bcdToDec(byte val)
// Convert binary coded decimal to normal decimal numbers
{
  return( (val/16*10) + (val%16) );
}

// ------------------------------------------------------------------------------

void readRTC(RTC_St *r)
// Read current date/time binary values and the temprerature (2 complement) from the DS3231 RTC
{
  byte    i;
  Wire.beginTransmission(DS3231_RTC);
  Wire.write(DS3231_SECRG);                       // Set the DS3231 Seconds Register
  Wire.endTransmission();
  // Read from RTC and convert to binary
  Wire.requestFrom(DS3231_RTC, 18);
  r->seconds = bcdToDec(Wire.read() & 0x7f);
  r->minutes = bcdToDec(Wire.read());
  r->hours = bcdToDec(Wire.read() & 0x3f);
  Wire.read();                                    // Jump over the DoW
  r->day = bcdToDec(Wire.read());
  r->month = bcdToDec(Wire.read());
  r->year = bcdToDec(Wire.read());
  for (i = 0; i < 10; i++) Wire.read();           // Jump over 10 registers
  r->tempC = Wire.read();
}

// ------------------------------------------------------------------------------

void writeRTC(RTC_St *r)
// Write given date/time binary values to the DS3231 RTC
{
  Wire.beginTransmission(DS3231_RTC);
  Wire.write(DS3231_SECRG);                       // Set the DS3231 Seconds Register
  Wire.write(decToBcd(r->seconds));
  Wire.write(decToBcd(r->minutes));
  Wire.write(decToBcd(r->hours));
  Wire.write(1);                                  // Day of week not used (always set to 1 = Sunday)
  Wire.write(decToBcd(r->day));
  Wire.write(decToBcd(r->month));
  Wire.write(decToBcd(r->year));
  Wire.endTransmission();

  // Reset the "Oscillator Stop Flag"
  Wire.beginTransmission(DS3231_RTC);
  Wire.write(DS3231_STATRG);                      // Set the DS3231 Status Register
  Wire.write(0x08);                               // Reset the "Oscillator Stop Flag" (32KHz output left enabled)
  Wire.endTransmission();
}

// ------------------------------------------------------------------------------

void RTCCheck(RTC_St *r)
// Check if the DS3231 RTC is present and set the date/time at compile date/time if 
// the RTC "Oscillator Stop Flag" is set (= date/time failure).
// Return value: 0 if RTC not present, 1 if found.
{
  r->foundRTC = 0;

  Wire.beginTransmission(DS3231_RTC);
  if (Wire.endTransmission() != 0)
    return;                                       // RTC not found

  r->foundRTC = 1;

  // Read the "Oscillator Stop Flag"
  Wire.beginTransmission(DS3231_RTC);
  Wire.write(DS3231_STATRG);                      // Set the DS3231 Status Register
  Wire.endTransmission();
  Wire.requestFrom(DS3231_RTC, 1);
  r->OscStopFlag = Wire.read() & 0x80;            // Read the "Oscillator Stop Flag"

  // Enable 1Hz output on INT_/SQW
  Wire.beginTransmission(DS3231_RTC);
  Wire.write(DS3231_CONTRG);                      // SQW enabled, 1Hz, default value for remaining bits
  Wire.write(0x00);
  Wire.endTransmission();
}

// ------------------------------------------------------------------------------

// Print to serial the current date/time
void printDateTime(RTC_St *r)
{
  print2digit(r->day);
  consolePrint("/");
  print2digit(r->month);
  consolePrint("/");
  print2digit(r->year);
  consolePrint(" ");
  print2digit(r->hours);
  consolePrint(":");
  print2digit(r->minutes);
  consolePrint(":");
  print2digit(r->seconds);
}

// ------------------------------------------------------------------------------

void print2digit(byte data)
// Print a byte [0..99] using 2 digit with leading zeros if needed
{
  if (data < 10) consolePrint("0");
  consolePrint("%u", data);
}

// ------------------------------------------------------------------------------

byte isLeapYear(byte yearXX)
// Check if the year 2000+XX (where XX is the argument yearXX [00..99]) is a leap year.
// Returns 1 if it is leap, 0 otherwise.
// This function works in the [2000..2099] years range. It should be enough...
{
  if (((2000 + yearXX) % 4) == 0) return 1;
  else return 0;
}

// ------------------------------------------------------------------------------

void ChangeRTC(void)
// Change manually the RTC Date/Time from keyboard
{
  byte    leapYear;   //  Set to 1 if the selected year is bissextile, 0 otherwise [0..1]
  char    inCharl;

  // Read RTC
  readRTC(&rtcData);

  // Change RTC date/time from keyboard
  byte tempByte = 0;
  consolePrint("\nIOS: RTC manual setting:\r\n");
  consolePrint("\nPress +/- or CR to accept\r\n");
  do
  {
    do
    {
      consolePrint(" ");
      switch (tempByte)
      {
        case 0:
          consolePrint("Year -> ");
          print2digit(rtcData.year);
        break;
        
        case 1:
          consolePrint("Month -> ");
          print2digit(rtcData.month);
        break;

        case 2:
          consolePrint("             \r Day -> ");
          print2digit(rtcData.day);
        break;

        case 3:
          consolePrint("Hours -> ");
          print2digit(rtcData.hours);
        break;

        case 4:
          consolePrint("Minutes -> ");
          print2digit(rtcData.minutes);
        break;

        case 5:
          consolePrint("Seconds -> ");
          print2digit(rtcData.seconds);
        break;
      }

      do
      {
        inCharl = WaitAndBlink(BLK);
      }
      while ((inCharl != '+') && (inCharl != '-') && (inCharl != 13));
      
      if (inCharl == '+')
      // Change units
        switch (tempByte)
        {
          case 0:
            rtcData.year++;
            if (rtcData.year > 99) rtcData.year = 0;
          break;

          case 1:
            rtcData.month++;
            if (rtcData.month > 12) rtcData.month = 1;
          break;

          case 2:
            rtcData.day++;
            if (rtcData.month == 2)
            {
              if (rtcData.day > (daysOfMonth[rtcData.month - 1] + isLeapYear(rtcData.year))) rtcData.day = 1;
            }
            else
            {
              if (rtcData.day > (daysOfMonth[rtcData.month - 1])) rtcData.day = 1;
            }
          break;

          case 3:
            rtcData.hours++;
            if (rtcData.hours > 23) rtcData.hours = 0;
          break;

          case 4:
            rtcData.minutes++;
            if (rtcData.minutes > 59) rtcData.minutes = 0;
          break;

          case 5:
            rtcData.seconds++;
            if (rtcData.seconds > 59) rtcData.seconds = 0;
          break;
        }
      if (inCharl == '-')
      // Change tens
        switch (tempByte)
        {
          case 0:
            if (rtcData.year == 0) rtcData.year = 99;
            else rtcData.year--;
          break;

          case 1:
            if (rtcData.month == 1) rtcData.month = 12;
            else rtcData.month--;
          break;

          case 2:
            if (rtcData.month == 2)
            {
              if (rtcData.day == 1) rtcData.day = daysOfMonth[rtcData.month - 1] + isLeapYear(rtcData.year);
              else rtcData.day--;
            }
            else
            {
              if (rtcData.day == 1) rtcData.day = daysOfMonth[rtcData.month - 1];
              else rtcData.day--;
            }
          break;

          case 3:
            if (rtcData.hours == 1) rtcData.hours = 23;
            else rtcData.hours--;
          break;

          case 4:
            if (rtcData.minutes == 1) rtcData.minutes = 59;
            else rtcData.minutes--;
          break;

          case 5:
            if (rtcData.seconds == 1) rtcData.seconds = 59;
            else rtcData.seconds--;
          break;
        }
      consolePrint("\r");
    }
    while (inCharl != 13);
    tempByte++;
  }
  while (tempByte < 6);  

  // Write new date/time into the RTC
  writeRTC(&rtcData);
  consolePrint(" ...done      \r\n\r\n");
  consolePrint("IOS: RTC date/time updated (");
  printDateTime(&rtcData);
  consolePrint(")\r\n");
}

// ------------------------------------------------------------------------------

void ChangeDefaultBANK(void)
// Change the power-on default RAM bank
{
  uint8_t prevBank = currentBank;
  char    inCharl;

  consolePrint("\nIOS: Power-on default RAM Bank setting:\r\n");
  consolePrint("\nPress +/- or CR to accept\r\n");
  do
  {
    do
    {
      consolePrint("Bank -> ");
      print2digit(currentBank);
      
      inCharl = WaitAndBlink(BLK);

      if (inCharl == '+' && currentBank < MAX_RAM_BANK)
        ++currentBank;
      if (inCharl == '-' && currentBank > 0)
        --currentBank;

      consolePrint("\r");
    }
    while ((inCharl != '+') && (inCharl != '-') && (inCharl != 13));
  }
  while (inCharl != 13);

  if (prevBank != currentBank)
  {
    EEPROM.update(EE_DEFBANK_ADDR, currentBank);
    SetRAMBank(currentBank);
  }
}

// ------------------------------------------------------------------------------

void ChangeZ80Clock(void)
{
  char    inCharl;

  consolePrint("\nIOS: Z80 Clock Setting:\r\n");
  consolePrint("\nPress +/- or CR to accept\r\n");

  uint8_t maxDivider = (F_CPU/(2*Z80_MIN_CLOCK))-1;                           // F_CPU / (2* min_freq(Hz)) -1 = divider

  do
  {
    do
    {
      float freq = (float)F_CPU/(2000000L*(clockDivider+1));
      consolePrint("Clock -> %d.%02d MHz\r", (int)freq, (int)((freq - (int)freq) * 100));
      
      inCharl = WaitAndBlink(BLK);

      if (inCharl == '-' && clockDivider < maxDivider)
        ++clockDivider;
      if (inCharl == '+' && clockDivider > 0)
        --clockDivider;

//      consolePrint("\r");
    }
    while ((inCharl != '+') && (inCharl != '-') && (inCharl != 13));
  }
  while (inCharl != 13);

  EEPROM.update(EE_CLOCKDIVIDER_ADDR, clockDivider);
}


// ============================================================
// EDIT NETWORK CONFIGURATION
// ============================================================
#define NETWORKMENU_ROWS  10
bool ChangeNetworkConfig(void)
{
  EEPROM.get(EEPROM_NETCFG_ADDR, netcfg);

  uint8_t cursor = 0;

  DrawNetworkTable(cursor, &netcfg);

  while (1)
  {
    char digit;
    uint8_t k = ReadKey(&digit);

    if (k == KEY_UP)
    {
      uint8_t old = cursor;
      cursor = (cursor == 0) ? NETWORKMENU_ROWS-1 : cursor - 1;
      RedrawRow(old, cursor, &netcfg);
      RedrawRow(cursor, cursor, &netcfg);
    }
    else if (k == KEY_DOWN)
    {
      uint8_t old = cursor;
      cursor = (cursor == NETWORKMENU_ROWS-1) ? 0 : cursor + 1;
      RedrawRow(old, cursor, &netcfg);
      RedrawRow(cursor, cursor, &netcfg);
    }
    else if (k == KEY_ESC)
    {
      // ESC = esci senza salvare
      EEPROM.get(EEPROM_NETCFG_ADDR, netcfg);                                 // ricarica configurazione originale
      DrawNetworkTable(cursor, &netcfg);                                      // ridisegna tutto
      consolePrint("\r\n\r\n\r\nIOS: Changes discarded.\r\n");
      return(false);                                                          // esci dall’editor
    }
    if (k == KEY_SAVE)
    {
      // S (uppercase) = save and exit
      consolePrint("\x1B[15;1HIOS: Type 'yes' to confirm         \r\n");
      uint8_t keySeq = 0;
      do
      {
        char k2 = WaitAndBlink(BLK);
        if ((keySeq == 0 && k2 == 'y') || (keySeq == 1 && k2 == 'e') || (keySeq == 2 && k2 == 's'))
          ++keySeq;
        else
          keySeq = 0xFF;
      }while(keySeq < 3);

      if (keySeq == 3)
      {
        netcfg.magic = NETCFG_MAGIC;
        EEPROM.put(EEPROM_NETCFG_ADDR, netcfg);
        consolePrint("\x1B[15;1HIOS: Network configuration updated. Restarting interface...\r\n");
        return(true);
      }
      else
      {
        consolePrint("\x1B[15;1HIOS: Network configuration skipped.\r\n");
      }
    }
    else if (k == KEY_ENTER)
    {
      switch (cursor)
      {
        case 0:
          EditMacInline(netcfg.mac, cursor);
        break;

        case 1:
          EditFieldInline(netcfg.ip, 4, cursor);
        break;

        case 2:
          EditFieldInline(netcfg.netmask, 4, cursor);
        break;

        case 3:
          EditFieldInline(netcfg.gateway, 4, cursor);
        break;

        case 4:
          EditFieldInline(netcfg.trusted_proxy, 4, cursor);
        break;

        case 5:
          EditFieldInline(netcfg.dns1, 4, cursor);
        break;

        case 6:
          EditFieldInline(netcfg.dns2, 4, cursor);
        break;

        case 7:
          consolePrint("\033[%u;%uH", 4 + cursor, 26);
          netcfg.tcpTimeout = ReadNumberFromConsole(netcfg.tcpTimeout);
        break;

        case 8:
          consolePrint("\033[%u;%uH", 4 + cursor, 26);
          netcfg.retries = ReadNumberFromConsole(netcfg.retries);
        break;

        case 9:
          char tmpstr[NETCFG_SOCK0_PWDLEN+1];
          strcpy(tmpstr, netcfg.sock0_password);
          consolePrint("\033[%u;%uH", 4 + cursor, 26);
          if (ReadStringFromConsole(tmpstr, NETCFG_SOCK0_PWDLEN))
            strcpy(netcfg.sock0_password, tmpstr);
        break;
      }

      // dopo qualsiasi edit: ridisegna la riga e rimetti il cursore a colonna 1
      RedrawRow(cursor, cursor, &netcfg);
      consolePrint("\033[%u;1H", 4 + cursor);
    }
  }
    return(false);
}

// ------------------------------------------------------------------------------

uint8_t ReadKey(char *digitOut)
{
  char c = WaitAndBlink(BLK);

  // ENTER
  if (c == 13)
    return KEY_ENTER;

  // BACKSPACE
  if (c == 0x08 || c == 0x7F)
    return KEY_BACKSPACE;

  // DECIMAL DIGIT
  if (c >= '0' && c <= '9')
  {
    *digitOut = c;
    return KEY_DIGIT;
  }

  // HEX LETTERS
  if ((c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))
  {
    *digitOut = c;
    return KEY_DIGIT;
  }

  // ESC
  if (c == 0x1B)
  {
    // wait to see if it's an ESC sequence or a single ESC keypress
    uint32_t start = millis();
    while (millis() - start < 100)
    {
      if (InputAvailable())
      {
        // other characters: ANSI escape sequence
        char c1 = WaitAndBlink(BLK);

        if (c1 != '[')
          return KEY_ESC;

        char c2 = WaitAndBlink(BLK);

        if (c2 == 'A') return KEY_UP;
        if (c2 == 'B') return KEY_DOWN;
        if (c2 == 'C') return KEY_RIGHT;
        if (c2 == 'D') return KEY_LEFT;

        return KEY_NONE;
      }
    }

    // no other characters: ESC
    return KEY_ESC;
  }

  if (c == 'S')
    return KEY_SAVE;

  return KEY_NONE;
}

// ------------------------------------------------------------------------------

void FormatIPv4(char *buf, uint8_t ip[4])
{
  sprintf(buf, "%3u.%3u.%3u.%3u", ip[0], ip[1], ip[2], ip[3]);
}

void FormatMAC(char *buf, uint8_t mac[6])
{
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void FormatNumber(char *buf, uint16_t n)
{
  sprintf(buf, "%u", n);
}

// ------------------------------------------------------------------------------

void PrintRow(uint8_t row, uint8_t cursor, const char *label, const char *value)
{
  uint8_t screenRow = 4 + row;

  // Vai alla riga corretta
  consolePrint("\033[%u;1H", screenRow);

  if (row == cursor)
  {
    // Riga selezionata: stampa, vai a capo, poi torna a colonna 1
    consolePrint("> %-20s | %s\r\n", label, value);
    consolePrint("\033[%u;1H", screenRow);
  }
  else
  {
    // Riga normale
    consolePrint("  %-20s | %s\r\n", label, value);
  }
}

// ------------------------------------------------------------------------------

void DrawNetworkTable(uint8_t cursor, NetConfig *net)
{
  char buf[32];

  consolePrint("\033[2J\033[H+--------------------------------------------------+\r\n");
  consolePrint("|              NETWORK CONFIGURATION               |\r\n");
  consolePrint("+----------------------+---------------------------+\r\n");

  for(uint8_t n = 0; n < NETWORKMENU_ROWS; ++n)
    RedrawRow(n, 0, net);

  consolePrint("\r\n+-----------------------------------------------------------------------------+\r\n");
  consolePrint("↑↓ sel.field/change   ←→ sel.byte   ENTER edit/confirm   S save   ESC disc.\r\n");
}

// ------------------------------------------------------------------------------

void RedrawRow(uint8_t rowIndex, uint8_t cursor, NetConfig *n)
{
  char buf[32];

  consolePrint("\033[%u;1H\033[K", 4 + rowIndex);

  switch (rowIndex)
  {
    case 0: FormatMAC(buf, n->mac); PrintRow(0, cursor, "MAC Address", buf); break;
    case 1: FormatIPv4(buf, n->ip); PrintRow(1, cursor, "IP Address", buf); break;
    case 2: FormatIPv4(buf, n->netmask); PrintRow(2, cursor, "Netmask", buf); break;
    case 3: FormatIPv4(buf, n->gateway); PrintRow(3, cursor, "Gateway", buf); break;
    case 4: FormatIPv4(buf, n->trusted_proxy); PrintRow(4, cursor, "Trusted Proxy", buf); break;
    case 5: FormatIPv4(buf, n->dns1); PrintRow(5, cursor, "DNS1", buf); break;
    case 6: FormatIPv4(buf, n->dns2); PrintRow(6, cursor, "DNS2", buf); break;
    case 7: FormatNumber(buf, n->tcpTimeout); PrintRow(7, cursor, "TCP Timeout (0.1ms)", buf); break;
    case 8: FormatNumber(buf, n->retries); PrintRow(8, cursor, "Retries", buf); break;
    case 9: PrintRow(9, cursor, "Socket0 Pwd", netcfg.sock0_password); break;
  }
}

// ------------------------------------------------------------------------------

void EditFieldInline(uint8_t *bytes, uint8_t count, uint8_t rowIndex)
{
  uint8_t cell = 0;
  char buf[6];
  uint8_t idx = 0;

  MoveCursorToField(rowIndex, cell, 0);

  while (1)
  {
    char digit;
    uint8_t k = ReadKey(&digit);

    switch (k)
    {
      case KEY_LEFT:
        if (cell > 0) { cell--; idx = 0; buf[0] = 0; }
      break;

      case KEY_RIGHT:
        if (cell < count - 1) { cell++; idx = 0; buf[0] = 0; }
      break;

      case KEY_UP:
        if (bytes[cell] < 255) bytes[cell]++;
      break;

      case KEY_DOWN:
        if (bytes[cell] > 0) bytes[cell]--;
      break;

      case KEY_SHIFT_UP:
        bytes[cell] = (bytes[cell] <= 245) ? bytes[cell] + 10 : 255;
      break;

      case KEY_SHIFT_DOWN:
        bytes[cell] = (bytes[cell] >= 10) ? bytes[cell] - 10 : 0;
      break;

      case KEY_DIGIT:
        if (idx >= sizeof(buf) - 1) { idx = 0; buf[0] = 0; }
        buf[idx++] = digit; buf[idx] = 0;
        bytes[cell] = (uint8_t)atoi(buf);
      break;

      case KEY_BACKSPACE:
        if (idx > 0) { idx--; buf[idx] = 0; bytes[cell] = atoi(buf); }
        else bytes[cell] = 0;
      break;

      case KEY_ENTER:
//        case KEY_ESC:
        return;
    }

    RedrawRow(rowIndex, rowIndex, &netcfg);
    MoveCursorToField(rowIndex, cell, 0);
  }
}

// ------------------------------------------------------------------------------

void EditMacInline(uint8_t *bytes, uint8_t rowIndex)
{
  uint8_t cell = 0;
  char buf[3];
  uint8_t idx = 0;

  buf[0] = 0;
  MoveCursorToField(rowIndex, cell, 1);

  while (1)
  {
    char digit;
    uint8_t k = ReadKey(&digit);

    switch (k)
    {
      case KEY_LEFT:
        if (cell > 0) { cell--; idx = 0; buf[0] = 0; }
      break;

      case KEY_RIGHT:
        if (cell < 5) { cell++; idx = 0; buf[0] = 0; }
      break;

      case KEY_UP:
        if (bytes[cell] < 0xFF) bytes[cell]++;
      break;

      case KEY_DOWN:
        if (bytes[cell] > 0x00) bytes[cell]--;
      break;

      case KEY_SHIFT_UP:
        bytes[cell] = (bytes[cell] <= 0xF0) ? bytes[cell] + 0x10 : 0xFF;
      break;

      case KEY_SHIFT_DOWN:
        bytes[cell] = (bytes[cell] >= 0x10) ? bytes[cell] - 0x10 : 0x00;
      break;

      case KEY_DIGIT:
        if (idx >= 2) { idx = 0; buf[0] = 0; }
        buf[idx++] = digit; buf[idx] = 0;
        bytes[cell] = (uint8_t)strtoul(buf, NULL, 16);
      break;

      case KEY_BACKSPACE:
        if (idx > 0) { idx--; buf[idx] = 0; bytes[cell] = strtoul(buf, NULL, 16); }
        else bytes[cell] = 0;
      break;

      case KEY_ENTER:
//      case KEY_ESC:
      return;
    }

    RedrawRow(rowIndex, rowIndex, &netcfg);
    MoveCursorToField(rowIndex, cell, 1);
  }
}

// ------------------------------------------------------------------------------

void MoveCursorToField(uint8_t rowIndex, uint8_t cell, uint8_t isMac)
{
    uint8_t screenRow = 4 + rowIndex;
    uint8_t offset = isMac ? 3 : 4;
    uint8_t baseCol = 26 + (cell * offset);
    uint8_t lsdCol = isMac ? (baseCol + 1) : (baseCol + 2);

    consolePrint("\033[%u;%uH", screenRow, lsdCol);
}

// ------------------------------------------------------------------------------

uint16_t ReadNumberFromConsole(uint16_t currentValue)
{
  char buf[6];        // max 5 cifre (0–65535)
  uint8_t idx = 0;

  while (true)
  {
    char c = WaitAndBlink(BLK);

    if (c == 0x0D)                                                            // ENTER → fine input
    {
      consolePrint("\r\n");

      if (idx == 0)                                                           // Se non è stato digitato nulla → restituisci il valore corrente
        return currentValue;

      buf[idx] = 0;
      return (uint16_t)atoi(buf);
    }

    // BACKSPACE
    if (c == 0x08 || c == 0x7F)
    {
      if (idx > 0)
      {
        idx--;
        consolePrint("\b \b");
      }
      continue;
    }

    if (c >= '0' && c <= '9')                                                 // Solo cifre
    {
      if (idx < sizeof(buf) - 1)
      {
        buf[idx++] = c;
        consolePrint("%c", c);
      }
      continue;
    }
    // Tutto il resto → ignorato
  }
}

// ------------------------------------------------------------------------------

bool ReadStringFromConsole(char* str, uint8_t len)
{
  uint8_t idx = 0;

  while (true)
  {
    char c = WaitAndBlink(BLK);

    if (c == 0x0D)                                                            // ENTER → fine input
    {
      consolePrint("\r\n");

      if (idx == 0)                                                           // Se non è stato digitato nulla, ritorna FALSE
        return (false);

      str[idx] = '\0';                                                        // termina la stringa
      return (true);
    }

    // BACKSPACE
    if (c == 0x08 || c == 0x7F)
    {
      if (idx > 0)
      {
        idx--;
        consolePrint("\b \b");
      }
      continue;
    }

    if (idx < len)
    {
      str[idx++] = c;
      consolePrint("%c", c);
    }
    continue;
  }
}



// ------------------------------------------------------------------------------

// Z80 bootstrap routines

// ------------------------------------------------------------------------------

void pulseClock(byte numPulse)
// Generate <numPulse> clock pulses on the Z80 clock pin.
// The steady clock level is LOW, e.g. one clock pulse is a 0-1-0 transition
{
  byte    i;
  for (i = 0; i < numPulse; i++)
  // Generate one clock pulse
  {
    // Send one impulse (0-1-0) on the CLK output
    digitalWrite(CLK, HIGH);
    digitalWrite(CLK, LOW);
  }
}

// ------------------------------------------------------------------------------

// Load a given byte to RAM using a sequence of two Z80 instructions forced on the data bus.
// The RAM_OE_DIS signal is used to force the RAM in HiZ, so the Atmega can write the needed instruction/data
//  on the data bus. Controlling the clock signal and knowing exactly how many clocks pulse are required it 
//  is possible control the whole loading process.
// Only the OE signal (output enable) is forced inactive, so the RAM is always writable with no need to cycle the enable.
// In the following "T" are the T-cycles of the Z80 (See the Z80 datashet).
// The two instruction are "LD (HL), n" and "INC (HL)".
//
// Modified for Z80_MBC2-V (WAIT_RES)
void loadByteToRAM(byte value)
{
  
  // Execute the LD(HL),n instruction (T = 4+3+3). See the Z80 datasheet and manual.
  // After the execution of this instruction the <value> byte is loaded in the memory address pointed by HL.
  digitalWrite(RAM_OE_DIS, HIGH);     // Force the RAM in HiZ (OE = HIGH) (moved here, before M1 cycle, that asserts MREQ+RD on memory
  pulseClock(1);                      // Execute the T1 cycle of M1 (Instruction Fetch machine cycle)
  DDRA = 0xFF;                        // Configure Z80 data bus D0-D7 (PA0-PA7) as output
  PORTA = LD_HL;                      // Write "LD (HL), n" instruction on data bus
  pulseClock(2);                      // Execute T2 and T3 cycles of M1
  DDRA = 0x00;                        // Configure Z80 data bus D0-D7 (PA0-PA7) as input... 
  PORTA = 0xFF;                       // ...with pull-up
  pulseClock(2);                      // Complete the execution of M1 and execute the T1 cycle of the following 
                                      // Memory Read machine cycle
  DDRA = 0xFF;                        // Configure Z80 data bus D0-D7 (PA0-PA7) as output
  PORTA = value;                      // Write the byte to load in RAM on data bus
  pulseClock(2);                      // Execute the T2 and T3 cycles of the Memory Read machine cycle
  DDRA = 0x00;                        // Configure Z80 data bus D0-D7 (PA0-PA7) as input... 
  PORTA = 0xFF;                       // ...with pull-up
//  digitalWrite(RAM_OE_DIS, LOW);      // Enable the RAM again (OE = Z80_RD)
  pulseClock(3);                      // Execute all the following Memory Write machine cycle

  // Execute the INC(HL) instruction (T = 6). See the Z80 datasheet and manual.
  // After the execution of this instruction HL points to the next memory address.
//  digitalWrite(RAM_OE_DIS, HIGH);     // Force the RAM in HiZ (OE = HIGH)
  pulseClock(1);                      // Execute the T1 cycle of M1 (Instruction Fetch machine cycle)
  DDRA = 0xFF;                        // Configure Z80 data bus D0-D7 (PA0-PA7) as output
  PORTA = INC_HL;                     // Write "INC(HL)" instruction on data bus
  pulseClock(2);                      // Execute T2 and T3 cycles of M1
  DDRA = 0x00;                        // Configure Z80 data bus D0-D7 (PA0-PA7) as input... 
  PORTA = 0xFF;                       // ...with pull-up
  pulseClock(3);                      // Execute all the remaining T cycles
  digitalWrite(RAM_OE_DIS, LOW);      // Enable the RAM again (OE = Z80_RD)
}

// ------------------------------------------------------------------------------
// Load "value" word into the HL registers inside the Z80 CPU, using the "LD HL,nn" instruction.
// In the following "T" are the T-cycles of the Z80 (See the Z80 datashet).
// Modified for Z80_MBC2-V.
void loadHL(word value)
{
  // Execute the LD dd,nn instruction (T = 4+3+3), with dd = HL and nn = value. See the Z80 datasheet and manual.
  // After the execution of this instruction the word "value" (16bit) is loaded into HL.
  digitalWrite(RAM_OE_DIS, HIGH);     // Force the RAM in HiZ (OE = HIGH) (moved here, before M1 cycle)
  pulseClock(1);                      // Execute the T1 cycle of M1 (Instruction Fetch machine cycle)
  DDRA = 0xFF;                        // Configure Z80 data bus D0-D7 (PA0-PA7) as output
  PORTA = LD_HLnn;                    // Write "LD HL, n" instruction on data bus
  pulseClock(2);                      // Execute T2 and T3 cycles of M1
  DDRA = 0x00;                        // Configure Z80 data bus D0-D7 (PA0-PA7) as input... 
  PORTA = 0xFF;                       // ...with pull-up
  pulseClock(2);                      // Complete the execution of M1 and execute the T1 cycle of the following 
                                      // Memory Read machine cycle
  DDRA = 0xFF;                        // Configure Z80 data bus D0-D7 (PA0-PA7) as output
  PORTA = lowByte(value);             // Write first byte of "value" to load in HL
  pulseClock(3);                      // Execute the T2 and T3 cycles of the first Memory Read machine cycle
                                      // and T1, of the second Memory Read machine cycle
  PORTA = highByte(value);            // Write second byte of "value" to load in HL
  pulseClock(2);                      // Execute the T2 and T3 cycles of the second Memory Read machine cycle                                    
  DDRA = 0x00;                        // Configure Z80 data bus D0-D7 (PA0-PA7) as input... 
  PORTA = 0xFF;                       // ...with pull-up
  digitalWrite(RAM_OE_DIS, LOW);      // Enable the RAM again (OE = Z80_RD)
}

// ------------------------------------------------------------------------------

void singlePulsesResetZ80(void)
// Reset the Z80 CPU using single pulses clock
{
  digitalWrite(RESET_, LOW);          // Set RESET_ active
  pulseClock(6);                      // Generate twice the needed clock pulses to reset the Z80
  digitalWrite(RESET_, HIGH);         // Set RESET_ not active
  pulseClock(2);                      // Needed two more clock pulses after RESET_ goes HIGH
}


// ------------------------------------------------------------------------------

// SD Disk routines (FAT16 and FAT32 filesystems supported) using the PetitFS library.
// For more info about PetitFS see here: http://elm-chan.org/fsw/ff/00index_p.html

// ------------------------------------------------------------------------------


byte mountSD(FATFS* fatFs)
// Mount a volume on SD: 
// *  "fatFs" is a pointer to a FATFS object (PetitFS library)
// The returned value is the resulting status (0 = ok, otherwise see printErrSD())
{
  return pf_mount(fatFs);
}

// ------------------------------------------------------------------------------

byte openSD(const char* fileName)
// Open an existing file on SD:
// *  "fileName" is the pointer to the string holding the file name (8.3 format)
// The returned value is the resulting status (0 = ok, otherwise see printErrSD())
{
  return pf_open(fileName);
}

// ------------------------------------------------------------------------------

byte readSD(void* buffSD, byte* numReadBytes)
// Read one "segment" (32 bytes) starting from the current sector (512 bytes) of the opened file on SD:
// *  "BuffSD" is the pointer to the segment buffer;
// *  "numReadBytes" is the pointer to the variables that store the number of read bytes;
//     if < 32 (including = 0) an EOF was reached).
// The returned value is the resulting status (0 = ok, otherwise see printErrSD())
//
// NOTE1: Each SD sector (512 bytes) is divided into 16 segments (32 bytes each); to read a sector you need to
//        to call readSD() 16 times consecutively
//
// NOTE2: Past current sector boundary, the next sector will be pointed. So to read a whole file it is sufficient 
//        call readSD() consecutively until EOF is reached
{
  UINT  numBytes;
  byte  errcode;
  errcode = pf_read(buffSD, 32, &numBytes);
  *numReadBytes = (byte) numBytes;
  return errcode;
}

// ------------------------------------------------------------------------------

byte writeSD(void* buffSD, byte* numWrittenBytes)
// Write one "segment" (32 bytes) starting from the current sector (512 bytes) of the opened file on SD:
// *  "BuffSD" is the pointer to the segment buffer;
// *  "numWrittenBytes" is the pointer to the variables that store the number of written bytes;
//     if < 32 (including = 0) an EOF was reached.
// The returned value is the resulting status (0 = ok, otherwise see printErrSD())
//
// NOTE1: Each SD sector (512 bytes) is divided into 16 segments (32 bytes each); to write a sector you need to
//        to call writeSD() 16 times consecutively
//
// NOTE2: Past current sector boundary, the next sector will be pointed. So to write a whole file it is sufficient 
//        call writeSD() consecutively until EOF is reached
//
// NOTE3: To finalize the current write operation a writeSD(NULL, &numWrittenBytes) must be called as last action
{
  UINT  numBytes;
  byte  errcode;
  if (buffSD != NULL)
  {
    errcode = pf_write(buffSD, 32, &numBytes);
  }
  else
  {
    errcode = pf_write(0, 0, &numBytes);
  }
  *numWrittenBytes = (byte) numBytes;
  return errcode;
}

// ------------------------------------------------------------------------------

byte seekSD(word sectNum)
// Set the pointer of the current sector for the current opened file on SD:
// *  "sectNum" is the sector number to set. First sector is 0.
// The returned value is the resulting status (0 = ok, otherwise see printErrSD())
//
// NOTE: "secNum" is in the range [0..16383], and the sector addressing is continuos inside a "disk file";
//       16383 = (512 * 32) - 1, where 512 is the number of emulated tracks, 32 is the number of emulated sectors
//
{
  byte i;
  return pf_lseek(((unsigned long) sectNum) << 9);
}

// ------------------------------------------------------------------------------

void printErrSD(byte opType, byte errCode, const char* fileName)
// Print the error occurred during a SD I/O operation:
//  * "OpType" is the operation that generated the error (0 = mount, 1= open, 2 = read,
//     3 = write, 4 = seek);
//  * "errCode" is the error code from the PetitFS library (0 = no error);
//  * "fileName" is the pointer to the file name or NULL (no file name)
//
// ........................................................................
//
// Errors legend (from PetitFS library) for the implemented operations:
//
// ------------------
// mountSD():
// ------------------
// NOT_READY
//     The storage device could not be initialized due to a hard error or no medium.
// DISK_ERR
//     An error occured in the disk read function.
// NO_FILESYSTEM
//     There is no valid FAT partition on the drive.
//
// ------------------
// openSD():
// ------------------
// NO_FILE
//     Could not find the file.
// DISK_ERR
//     The function failed due to a hard error in the disk function, a wrong FAT structure or an internal error.
// NOT_ENABLED
//     The volume has not been mounted.
//
// ------------------
// readSD() and writeSD():
// ------------------
// DISK_ERR
//     The function failed due to a hard error in the disk function, a wrong FAT structure or an internal error.
// NOT_OPENED
//     The file has not been opened.
// NOT_ENABLED
//     The volume has not been mounted.
// 
// ------------------
// seekSD():
// ------------------
// DISK_ERR
//     The function failed due to an error in the disk function, a wrong FAT structure or an internal error.
// NOT_OPENED
//     The file has not been opened.
//
// ........................................................................
{
  if (errCode)
  {
    consolePrint("\r\nIOS: SD error %u (",errCode);
    switch (errCode)
    // See PetitFS implementation for the codes
    {
      case 1: consolePrint("DISK_ERR"); break;
      case 2: consolePrint("NOT_READY"); break;
      case 3: consolePrint("NO_FILE"); break;
      case 4: consolePrint("NOT_OPENED"); break;
      case 5: consolePrint("NOT_ENABLED"); break;
      case 6: consolePrint("NO_FILESYSTEM"); break;
      default: consolePrint("UNKNOWN"); 
    }
    consolePrint(" on ");
    switch (opType)
    {
      case 0: consolePrint("MOUNT"); break;
      case 1: consolePrint("OPEN"); break;
      case 2: consolePrint("READ"); break;
      case 3: consolePrint("WRITE"); break;
      case 4: consolePrint("SEEK"); break;
      default: consolePrint("UNKNOWN");
    }
    consolePrint(" operation");
    if (fileName)
    // Not a NULL pointer, so print file name too
    {
      consolePrint(" - File: %s", fileName);
    }
    consolePrint(")\r\n");
  }
}

// ------------------------------------------------------------------------------

void waitKey(void)
// Wait a key to continue
{
  FlushSerials();                                                             // flush serial (always) and telnet (if bridge is enabled)

  consolePrint("IOS: Check SD and press a key to repeat\r\n\r\n");
  while(Serial.available() < 1 &&
      ((telnet_sessions[0].rx_head == telnet_sessions[0].rx_tail) || !(telnet_sessions[0].telnet_flags & TFLAG_CON_BRIDGE)))
  {
    if (eth_ok)
    {
      telnet_handler();
    }
  }
}

// ------------------------------------------------------------------------------

void printOsName(byte currentDiskSet)
// Print the current Disk Set number and the OS name, if it is defined.
// The OS name is inside the file defined in DS_OSNAME
{
  consolePrint("Disk Set %u", currentDiskSet);
  OsName[2] = currentDiskSet + 48;    // Set the Disk Set
  openSD(OsName);                     // Open file with the OS name
  readSD(bufferSD, &numReadBytes);    // Read the OS name
  if (numReadBytes > 0)
  // Print the OS name
  {
    consolePrint(" (%s)", (const char *)bufferSD);
  }
}

// End of known world for this code - Hic Sunt Leones!
