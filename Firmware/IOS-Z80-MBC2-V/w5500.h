#ifndef W5500_H
#define W5500_H

#include <stdint.h>

// ---------------------------------------------------------
//  DEFINES
// ---------------------------------------------------------
#define MAX_TELNET_SESSIONS  2            // Max simultaneous telnet sessions (sockets). PLEASE CHECK 'BUFFER SIZE DEFS AND MASKS' section

#define TELNET_SPECIAL_CMDS   "\r\nCommands available with Z80 in RUN state\r\n" \
                                "(\"E\" and \"R\" always enabled):\r\n\r\n" \
                                "[pwd]M  Back to Main Menu\r\n" \
                                "[pwd]Z  Z80 Hard Reset\r\n" \
                                "[pwd]I  Enable Tick INT\r\n" \
                                "[pwd]D  Disable Tick INT\r\n" \
                                "[pwd]E  Ethernet Reinit\r\n" \
                                "[pwd]R  Board Hard Reset\r\n\r\n"

#define TELNET_BUF_SIZE         1536      // software FIFO size
#define ZOMBIE_TIMEOUT          2400000UL // 40 minutes base timeout

#define NETCFG_MAGIC            42
#define NETCFG_SOCK0_PWDLEN     20

#define PROXYIPBUF_LEN          15

#define ETH_CS_                 11        // PD3 pin 17   ETH SPI CHIP SELECT (active low)
#define ETH_CS_PORT             PORTD     // ETH_CS_ port register
#define ETH_CS_PIN              3

#define SS_                     4         // PB4 pin 5    SD SPI CHIP SELECT (active low)
#define SS_PORT                 PORTB     // SS_ port register
#define SS_PINPORT              PINB      // SS_ physical electrical pin state register
#define SS_PIN                  4

#define CMD_IDLE                0
#define CMD_MAIN_MENU           1
#define CMD_Z80_RESET           2
#define CMD_ENABLE_TICK_INT     3
#define CMD_DISABLE_TICK_INT    4

// ---------------------------------------------------------
//  BITS IN 'telnet_flags'
// ---------------------------------------------------------
#define TFLAG_RAW_MODE          1         // '1' = RAW mode active
#define TFLAG_CON_BRIDGE        2         // '1' = socket 0   : data from/to telnet is treated by IOS as received/sent from/to serial (remote console)
                                          //     = socket >= 1: data received / sent is also forwarded to socket 0 / serial
#define TFLAG_DISCONNECT        4         // '1' = client will be dropped
#define TFLAG_PURGETXONCONN     8         // '1' = TX ring buffer is cleared on new connection
#define TFLAG_FORCE_NEGO_C_M    0x10      // '1' = calls 'telnet_negotiate_character_mode' on any new connection, regardless of telnet / non telnet client type

// ---------------------------------------------------------
// STRUCTURES
// ---------------------------------------------------------

struct NetConfig {
    uint8_t mac[6];
    uint8_t ip[4];
    uint8_t netmask[4];
    uint8_t gateway[4];
    uint8_t trusted_proxy[4];
    uint8_t dns1[4];
    uint8_t dns2[4];
    uint16_t tcpTimeout;
    uint8_t retries;
    char sock0_password[NETCFG_SOCK0_PWDLEN + 1];
    uint8_t magic;
};

// Struttura sessione TELNET (multi-socket)
typedef struct {
    uint8_t   sn;                         // socket number (0..7)
    uint16_t  port;                       // TCP port

    uint8_t   last_state;                 // SOCK_*
    uint8_t   telnet_cmd_state;
    uint8_t   telnet_flags;
    uint8_t   iac_state;
    uint8_t   is_telnet;
    uint8_t   send_banner;
    uint8_t   send_iac;
    uint8_t   send_mini_iac;

    // timeout
    uint32_t  last_rx_time;
    uint32_t  timeout_val;

    // FIFO RX/TX
    uint8_t   rx_buf[TELNET_BUF_SIZE];
    uint16_t  rx_head;
    uint16_t  rx_tail;

    uint8_t   tx_buf[TELNET_BUF_SIZE];
    uint16_t  tx_head;
    uint16_t  tx_tail;

    uint8_t   remoteIP[4];

    // proxy message parser state machine (real caller IP)
    uint8_t proxy_state;                  // 0 = IDLE, 1 = PARSING, 2 = DONE
    uint8_t proxy_ptr;
    uint8_t proxy_space_count;
    char proxy_ip_buf[PROXYIPBUF_LEN + 1];
} TelnetSession;

// ---------------------------------------------------------
//  PUBLIC FUNCTION PROTOTYPES
// ---------------------------------------------------------
void w5500_check(void);
void w5500_init(void);
void telnet_handler(void);
void telnet_tx_push(uint8_t, uint8_t);
bool telnetClientConnected(uint8_t);

#endif
