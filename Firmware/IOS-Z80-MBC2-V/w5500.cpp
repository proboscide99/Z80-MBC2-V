#include <avr/io.h>
#include <Arduino.h>
#include "w5500.h"
#include <avr/wdt.h>

#define   DEBUG             1                             // Debug: 0 = Off, 1 = Light, 2+ = Exhaustive

//
// Ottimizzazioni:
// v Aggiungere << 3 nelle tre formule dei block select ed eliminare i corrispondenti shift nelle wizWrite, wizRead, wizBurstWrite
// v scrivere una wizRead16 che legga un dato a 16 bit in una sola transazione SPI
// v scrivere una wizWrite16 che scriva un dato a 16 bit in una sola transazione SPI
// v Riattivare il setup dei registri RTR0 e RCR?
// v Allargare i buffer del wix5500 per usare almeno 4KB (ancora compatibile con 4 socket) se non 8KB (ok con gli attuali due socket utilizzati)?
//


// ---------------------------------------------------------
//  DEFINIZIONI HARDWARE
// ---------------------------------------------------------
#define ETH_SELECT()    (ETH_CS_PORT &= ~(1 << ETH_CS_PIN))
#define ETH_DESELECT()  (ETH_CS_PORT |=  (1 << ETH_CS_PIN))

// ---------------------------------------------------------
//  W5500 COMMON REGISTERS
// ---------------------------------------------------------
#define MR                  0x0000
#define GAR0                0x0001
#define SUBR0               0x0005
#define SHAR0               0x0009
#define SIPR0               0x000F
#define RTR0                0x0019                        // Retry Time-value Register (MSB)
#define RCR                 0x001B                        // Retry Count Register
// ---------------------------------------------------------
//  SOCKET REGISTERS
// ---------------------------------------------------------
#define Sn_MR               0x0000
#define Sn_CR               0x0001
#define Sn_SR               0x0003
#define Sn_PORT             0x0004
#define Sn_DIPR             0x000C

#define Sn_RXBUF_SIZE       0x001E
#define Sn_TXBUF_SIZE       0x001F

#define Sn_TX_FSR           0x0020
#define Sn_TX_RD            0x0022
#define Sn_TX_WR            0x0024

#define Sn_RX_RSR           0x0026
#define Sn_RX_RD            0x0028

#define Sn_KPALVTR          0x002F

// ---------------------------------------------------------
//  BUFFER SIZE DEFS AND MASKS
//  2 SOCKET: MAX 8KB;
//  4 SOCKET: MAX 4KB;
//  8 SOCKET: MAX 2KB.
// ---------------------------------------------------------
#define S_TXBUF_MASK       0x0FFF                         // 4KB
#define S_RXBUF_MASK       0x0FFF                         // 4KB
#define BUF_SIZE_4K         0x04

// ---------------------------------------------------------
//  BLOCK SELECT PER SOCKET n
// ---------------------------------------------------------
//#define S_REG(sn)   (((uint8_t)(sn) << 2) + 1)
//#define S_TXB(sn)   (((uint8_t)(sn) << 2) + 2)
//#define S_RXB(sn)   (((uint8_t)(sn) << 2) + 3)

#define S_REG(sn)   ((((uint8_t)(sn) << 2) + 1) << 3)
#define S_TXB(sn)   ((((uint8_t)(sn) << 2) + 2) << 3)
#define S_RXB(sn)   ((((uint8_t)(sn) << 2) + 3) << 3)
/*
  00000 Selects Common Register.
  00001 Selects Socket 0 Register
  00010 Selects Socket 0 TX Buffer
  00011 Selects Socket 0 RX Buffer
  00100 Reserved
  00101 Selects Socket 1 Register
  00110 Selects Socket 1 TX Buffer
  00111 Selects Socket 1 RX Buffer
  01000 Reserved
  [...]
  11101 Selects Socket 7 Register
  11110 Selects Socket 7 TX Buffer
  11111 Selects Socket 7 RX Buffer

  Prima della trasmissione del byte, questi valori vengono shiftati << 3, portando a questi indirizzi finali sul byte SPI:

  Socket 0: Registers=0x08, TX=0x10, RX=0x18
  Socket 1: Registers=0x28, TX=0x30, RX=0x38
  Socket 2: Registers=0x48, TX=0x50, RX=0x58
  Socket 3: Registers=0x68, TX=0x70, RX=0x78
  Socket 4: Registers=0x88, TX=0x90, RX=0x98
  Socket 5: Registers=0xA8, TX=0xB0, RX=0xB8
  Socket 6: Registers=0xC8, TX=0xD0, RX=0xD8
  Socket 7: Registers=0xE8, TX=0xF0, RX=0xF8

  Pertanto definiamo le costanti gia' shiftate << 3 nella formula, cosi' risparmiamo di farlo fare al processore.
*/

// ---------------------------------------------------------
// Socket Status Register values (Sn_SR)
// ---------------------------------------------------------
#define SOCK_CLOSED         0x00
#define SOCK_INIT           0x13
#define SOCK_LISTEN         0x14
#define SOCK_SYNSENT        0x15
#define SOCK_SYNRECV        0x16
#define SOCK_ESTABLISHED    0x17
#define SOCK_FIN_WAIT       0x18
#define SOCK_CLOSING        0x1A
#define SOCK_TIME_WAIT      0x1B
#define SOCK_CLOSE_WAIT     0x1C
#define SOCK_LAST_ACK       0x1D
#define SOCK_UDP            0x22
#define SOCK_IPRAW          0x32
#define SOCK_MACRAW         0x42
#define SOCK_PPPOE          0x5F

// ---------------------------------------------------------
// Socket Command Register values (Sn_CR)
// ---------------------------------------------------------
#define CR_DISCON           0x08
#define CR_CLOSE            0x10
#define CR_SEND             0x20
#define CR_RECV             0x40

// ---------------------------------------------------------
// PROXY state machine (real caller's IP detection)
// ---------------------------------------------------------
#define PROXYSTATE_IDLE     0
#define PROXYSTATE_PARSING  1
#define PROXYSTATE_PROXIED  2

const char PROXY_MAGIC[] = "PROXY TCP";

// ---------------------------------------------------------
//  LOCAL FUNCTIONS
// ---------------------------------------------------------
static uint8_t wizRead(uint16_t, uint8_t);
static uint16_t wizRead16(uint16_t, uint8_t);
static void wizBurstRead(uint16_t, uint8_t, uint8_t *, uint16_t);
static void wizWrite(uint16_t, uint8_t block, uint8_t);
static void wizWrite16(uint16_t, uint8_t block, uint16_t);
static void wizBurstWrite(uint16_t, uint8_t, const uint8_t *, uint16_t);
static uint8_t w5500_read_rx_byte(TelnetSession *, uint16_t);
static int8_t w5500_socket_listen(TelnetSession *);
static uint8_t wait_command_done(uint8_t);
static bool w5500_software_reset(void);
static uint8_t spiTransfer(uint8_t);
static bool telnet_rx_push(TelnetSession *, uint8_t);
static void telnet_negotiate_character_mode(TelnetSession *);
static void telnet_write_char(TelnetSession *, uint8_t);
static void telnet_tx_pop(TelnetSession *);
static void telnet_write_block(TelnetSession *, const uint8_t *, uint16_t);
static void w5500_disconnect(TelnetSession *);
static void systemResetSafe(void);
static void telnet_debug_regs(TelnetSession *);
static void telnet_struct_init(void);
static bool parse_ip_string(char*, uint8_t*);

// ---------------------------------------------------------
// VARIABLES
// ---------------------------------------------------------
NetConfig netcfg;
TelnetSession telnet_sessions[MAX_TELNET_SESSIONS];
bool eth_ok = false;

extern byte telnetSpecialCmds;
extern byte inTheMenu;

// ---------------------------------------------------------
//  TELNET INIT
//  Sets 'eth_ok' true if success.
//
//  NOTE: The caller should set MOSI/SCK as outputs
//        before calling this function.
//
// ---------------------------------------------------------
void w5500_check(void)
{
  eth_ok = false;

  // SPI enable, master, mode 0, F_CPU/2 (may be overridden by sd-card init)
  SPCR = (1 << SPE) | (1 << MSTR);
  SPSR = (1 << SPI2X);

  // Reset chip
  wizWrite(MR, 0x00, 0x80);
  delay(100);

  uint8_t ver = wizRead(0x0039, 0x00);                                          // read VERSIONR (0x0039, block 0)

  if (ver != 0x04)
  {
    Serial.print(F("[ERR] W5500 not found! VERSIONR = "));
    Serial.println(ver, HEX);
    return;
  }

  eth_ok = true;
}


// ---------------------------------------------------------
//  W5500 INIT
// ---------------------------------------------------------
void w5500_init(void)
{
  if (!eth_ok)
    return;

  eth_ok = w5500_software_reset();

  if (!eth_ok)
    return;

  // MAC
  wizBurstWrite(SHAR0, 0x00, netcfg.mac, 6);

  // IP
  wizBurstWrite(SIPR0, 0x00, netcfg.ip, 4);

  // Subnet
  wizBurstWrite(SUBR0, 0x00, netcfg.netmask, 4);

  // Gateway
  wizBurstWrite(GAR0, 0x00, netcfg.gateway, 4);

  // Timeout (RTR) (*100us per cui 2000 = 200ms)
  wizWrite16(RTR0, 0x00, netcfg.tcpTimeout); 

  // Retries (RCR)
  wizWrite(RCR, 0x00, netcfg.retries);

  for (uint8_t i = 0; i < MAX_TELNET_SESSIONS; i++)
  {
    // Buffer: 4KB RX, 4KB TX
    wizWrite(Sn_RXBUF_SIZE, S_REG(i), BUF_SIZE_4K);
    wizWrite(Sn_TXBUF_SIZE, S_REG(i), BUF_SIZE_4K);
  }

  telnet_struct_init();

  for (uint8_t i = 0; i < MAX_TELNET_SESSIONS; i++)
    w5500_socket_listen(&telnet_sessions[i]);
}

// ---------------------------------------------------------
//  INIT TELNET STRUCT
// ---------------------------------------------------------
static void telnet_struct_init(void)
{
  for (uint8_t i = 0; i < MAX_TELNET_SESSIONS; i++) {
    TelnetSession *s = &telnet_sessions[i];

    s->sn = i;
    s->port = 2300+i;
    s->last_state       = SOCK_CLOSED;
    s->telnet_cmd_state = 0;
    s->telnet_flags     = 0;
    s->iac_state        = 0;
    s->is_telnet        = false;
    s->send_banner      = true;
    s->send_iac         = false;                                                // don't send IAC by default
    s->send_mini_iac    = false;
    s->last_rx_time     = millis();
    s->timeout_val      = (i == 0) ? (ZOMBIE_TIMEOUT << 2) : ZOMBIE_TIMEOUT;    // timeout quadruplo per il socket 0

    s->rx_head = s->rx_tail = 0;
    s->tx_head = s->tx_tail = 0;

    s->proxy_state = PROXYSTATE_IDLE;
  }
}

// ---------------------------------------------------------
//  TCP SOCKET LISTEN
//
//  Returns:
//   0 = OK
//  -1 = Hardware error (SPI timeout reading Sn_CR register)
//  -2 = socket not in 'SOCK_INIT' state
//  -3 = socket not in 'SOCK_LISTEN' state
// ---------------------------------------------------------
static int8_t w5500_socket_listen(TelnetSession *s)
{
  wizWrite(Sn_CR, S_REG(s->sn), 0x10);                                          // CLOSE
  if (wait_command_done(s->sn))
  {
    Serial.printf("[W5500] CLOSE timeout on socket %u\r\n", s->sn);
    return -1;
  }
  
  wizWrite(Sn_MR, S_REG(s->sn), 0x01);                                          // TCP

  wizWrite16(Sn_PORT, S_REG(s->sn), s->port);                                   // PORT

  wizWrite(Sn_CR, S_REG(s->sn), 0x01);                                          // OPEN
  if (wait_command_done(s->sn))
  {
    Serial.printf("[W5500] OPEN timeout on socket %u\r\n", s->sn);
    return -1;
  }

  if (wizRead(Sn_SR, S_REG(s->sn)) != SOCK_INIT)
  {
    Serial.printf("[W5500] SOCK_INIT error on socket %u\r\n", s->sn);
    return -2; 
  }

  wizWrite(Sn_KPALVTR, S_REG(s->sn), 0x0C);                                     // keep alive (5 sec * 0x0C = 1 minute)

  wizWrite(Sn_CR, S_REG(s->sn), 0x02);                                          // LISTEN
  if (wait_command_done(s->sn))
  {
    Serial.printf("[W5500] LISTEN timeout on socket %u\r\n", s->sn);
    return -1;
  }

  if (wizRead(Sn_SR, S_REG(s->sn)) != SOCK_LISTEN)
  {
    Serial.printf("[W5500] SOCK_LISTEN error on socket %u\r\n", s->sn);
    return -3; 
  }
  return 0;
}


// Restituisce 0 se OK, 1 se TIMEOUT
static uint8_t wait_command_done(uint8_t sn)
{
  uint32_t timeout = 10000;

  while (wizRead(Sn_CR, S_REG(sn)) != 0)
  {
    if (--timeout == 0)
      return 1;
  }
#if DEBUG > 1
  Serial.printf("[W5500] cmd_done: remaining timeout %u\r\n", timeout);
#endif
  return 0;
}


// ---------------------------------------------------------
//  SOCKET POLLING AND TELNET PARSER
// ---------------------------------------------------------
void telnet_handler(void)
{
  if (!eth_ok)
    return;

  // ---------------------------------------------------------
  //  SPI ARBITRATION: avoid conflicts with SD card
  // ---------------------------------------------------------
  if (!((SS_PORT & SS_PINPORT) & (1 << SS_PIN)))
  {
#if DEBUG > 1
    Serial.println(F("[W5500] SS_ conflict !!!"));
#endif
    return;
  }

  for (uint8_t i = 0; i < MAX_TELNET_SESSIONS; i++)
  {
    TelnetSession *s = &telnet_sessions[i];

    // ---------------------------------------------------------
    //  TX FIFO → send bytes if queued and client connected
    // ---------------------------------------------------------
    if (s->last_state == SOCK_ESTABLISHED && s->tx_head != s->tx_tail)
      telnet_tx_pop(s);

    // ---------------------------------------------------------
    //  EDGE DETECTION: new connection established / disconnect
    // ---------------------------------------------------------
    uint8_t st = wizRead(Sn_SR, S_REG(s->sn));

    if (st == SOCK_ESTABLISHED && s->last_state != SOCK_ESTABLISHED)
    {
      wizBurstRead(Sn_DIPR, S_REG(s->sn), s->remoteIP, 4);                        // stores the remote IP address

#if DEBUG > 0
      Serial.printf("[TELNET] Socket %u connected to: %u.%u.%u.%u\r\n", 
                s->sn, s->remoteIP[0], s->remoteIP[1], 
                s->remoteIP[2], s->remoteIP[3]);
#endif
      // Proxy IP detection: is the client's IP equal to the configured PROXY IP?
      if (s->remoteIP[0] == netcfg.trusted_proxy[0] && s->remoteIP[1] == netcfg.trusted_proxy[1] && 
          s->remoteIP[2] == netcfg.trusted_proxy[2] && s->remoteIP[3] == netcfg.trusted_proxy[3]) 
      {
        // YES: we need to check for a 'PROXY Protocol V1 string' carrying the real client's IP
#if DEBUG > 0
        Serial.print("[PARSER] Proxy IP, parsing: ");
#endif        
        s->proxy_state = PROXYSTATE_PARSING;
        s->proxy_ptr = 0;
        s->proxy_space_count = 0;
      }
      else
        s->proxy_state = PROXYSTATE_IDLE;

      s->last_rx_time = millis();
      s->send_banner = true;                                                      // banner will be sent
      s->iac_state = 0;
      s->is_telnet = false;                                                       // by default, not a telnet client (raw client)
      s->telnet_flags &= ~TFLAG_DISCONNECT;                                       // clears any stale DISCONNECT REQUEST flag

      if (!(s->telnet_flags & TFLAG_FORCE_NEGO_C_M))                              // if IAC negotiation is not forced by 'TFLAG_FORCE_NEGO_C_M' flag:
      {
        s->send_iac = false;                                                      // we won't send the FULL IAC negotiation
        s->send_mini_iac = true;                                                  // will send mini IAC (to probe telnet clients)
      }
      else
      {
        s->send_iac = true;                                                       // IAC forced by 'TFLAG_FORCE_NEGO_C_M' flag
        s->send_mini_iac = false;
      }

      if (s->telnet_flags & TFLAG_PURGETXONCONN)                                  // if option enabled,
      {                                                                           // purges the TX FIFO
//        Serial.printf("head %u, tail %u purged\r\n", s->tx_head, s->tx_tail);
        s->tx_tail = s->tx_head;
      }
    }

    if (st != SOCK_ESTABLISHED && s->last_state == SOCK_ESTABLISHED)
    {
#if DEBUG > 0
      Serial.print(F("[TELNET] Disconnected on socket "));
      Serial.println(s->sn);
#endif

      s->telnet_flags &= ~(TFLAG_RAW_MODE | TFLAG_DISCONNECT);                    // terminates RAW mode and deactivates DISCONNECT REQUEST flag
      if (s->sn == 0)
        s->telnet_flags &= ~TFLAG_CON_BRIDGE;                                     // on socket 0, also terminates IOS bridging to disable remote console
    }

    s->last_state = st;

    // ---------------------------------------------------------
    // PROXY PARSING TIMEOUT
    // ---------------------------------------------------------
    if (s->proxy_state == PROXYSTATE_PARSING)
    {
      if ((millis() - s->last_rx_time) > 1000)
      {
#if DEBUG > 0
        Serial.println("--->timeout");
#endif        
        s->proxy_state = PROXYSTATE_IDLE;
      }
    }

    // ---------------------------------------------------------
    //  AUTO-RECOVERY: socket state handling
    // ---------------------------------------------------------

    // CLOSED → LISTEN
    if (st == SOCK_CLOSED)
    {
#if DEBUG > 0
      Serial.print(F("[TELNET] CLOSED → LISTEN on socket "));
      Serial.println(s->sn);
#endif
      s->last_rx_time = millis();
      w5500_socket_listen(s);
      return;
    }

    // CLOSE_WAIT → close and reopen
    if (st == SOCK_CLOSE_WAIT)
    {
#if DEBUG > 0
      Serial.print(F("[TELNET] CLOSE_WAIT → CLOSE on socket "));
      Serial.println(s->sn);
#endif
//      wizWrite(Sn_CR, S_REG(s->sn), 0x10);  // CLOSE (gia' fatto in w5500_socket_listen())
//      while (wizRead(Sn_CR, S_REG(s->sn)));
      s->last_rx_time = millis();
      w5500_socket_listen(s);
      continue;
    }

    // closing
    if (st == SOCK_CLOSING || st == SOCK_TIME_WAIT || st == SOCK_FIN_WAIT || st == SOCK_CLOSE_WAIT)
      continue;

    // ZOMBIE TIMEOUT / disconnect request
    if (st == SOCK_ESTABLISHED)
    {
      if ((millis() - s->last_rx_time) > s->timeout_val ||                        // se tempo idle scaduto
          (s->telnet_flags & TFLAG_DISCONNECT))                                   // o disconnect richiesto dall'esterno...
      {
#if DEBUG > 0
        if (s->telnet_flags & TFLAG_DISCONNECT)
          Serial.print(F("[TELNET] Hangup request: → resetting socket "));
        else
          Serial.print(F("[TELNET] Zombie timeout: → resetting socket "));
        Serial.println(s->sn);
#endif
        w5500_disconnect(s);
        s->last_rx_time = millis();
        w5500_socket_listen(s);
        continue;
      }
    }
    else
    {
//      s->send_banner = true;
      continue;
    }

    // ---------------------------------------------------------
    //  SEND character_mode negotiation IACs to telnet clients
    // ---------------------------------------------------------
    if (s->send_iac && (s->proxy_state != PROXYSTATE_PARSING))
    {
#if DEBUG > 0
      Serial.print(F("[TELNET] Sending IAC on socket "));
      Serial.println(s->sn);
#endif
      telnet_negotiate_character_mode(s);                                         // client: immediate mode (don't wait for CR to send characters), no local echo
      s->send_iac = false;
    }

    // ---------------------------------------------------------
    //  SEND IAC DO SUPPRESS-GO-AHEAD (mini IAC)
    // ---------------------------------------------------------
    if (s->send_mini_iac && (s->proxy_state != PROXYSTATE_PARSING))
    {
#if DEBUG > 0
      Serial.print(F("[TELNET] Sending mini IAC on socket "));
      Serial.println(s->sn);
#endif
      telnet_write_char(s, 0xFF);                                               // Sends IAC DO SUPPRESS-GO-AHEAD (0xFF 0xFD 0x03)
      telnet_write_char(s, 0xFD);                                               // to see if a telnet client responds
      telnet_write_char(s, 0x03);
      s->send_mini_iac = false;
    }

    // ---------------------------------------------------------
    //  SEND BANNER ONCE (socket 0: only if BRIDGE is active)
    // ---------------------------------------------------------
    if ((s->send_banner && (s->sn > 0 || (s->telnet_flags & TFLAG_CON_BRIDGE))) &&
        (s->proxy_state != PROXYSTATE_PARSING))
    {
      static const char banner_top[] = "\r\n"
        "*****************************************\r\n"
        "*         Welcome to Z80_MBC2-V         *\r\n";

      static const char banner_halted[] =
        "*             Z80 is Halted             *\r\n";

      static const char banner_bottom[] =
        "*****************************************\r\n\r\n";

#if DEBUG > 0
      Serial.print(F("[TELNET] Sending banner on socket "));
      Serial.println(s->sn);
#endif

      delay(500);
      telnet_write_block(s, (const uint8_t*)banner_top, strlen(banner_top));
      if (inTheMenu != 0)
        telnet_write_block(s, (const uint8_t*)banner_halted, strlen(banner_halted));
      telnet_write_block(s, (const uint8_t*)banner_bottom, strlen(banner_bottom));

      if (s->sn == 0)
        telnet_write_block(s, (const uint8_t*)TELNET_SPECIAL_CMDS, strlen(TELNET_SPECIAL_CMDS));

      s->send_banner = false;
    }

    // ---------------------------------------------------------
    //  RX HANDLER (BYTES RECEIVED FROM TELNET CLIENT)
    // ---------------------------------------------------------
    uint16_t rx = wizRead16(Sn_RX_RSR, S_REG(s->sn));
    if (rx == 0)
      continue;

    s->last_rx_time = millis();

    uint16_t rd = wizRead16(Sn_RX_RD, S_REG(s->sn));

#if DEBUG > 1
    telnet_debug_regs(s);
#endif

    uint8_t safety = 255;
    bool pushok = true;
    while (rx-- && safety-- && pushok)
    {
      uint8_t b = w5500_read_rx_byte(s, rd);

#if DEBUG > 1
      Serial.printf("TELNET RX: %02X  iac=%d  istelnet=%d  cmd=%d  flags=%d, socket=%d\r\n",
          b, s->iac_state, s->is_telnet, s->telnet_cmd_state, s->telnet_flags, s->sn);
#endif

      // ----------------------------------------------------------------
      // PROXY PARSER:
      //
      // If the source IP matched the configured "Trusted Proxy",
      // we may receive a PROXY Protocol V1 string with the real caller's
      // IP address.
      //
      // The string from the proxy will be like:
      // PROXY TCP6 95.255.175.42:52880-184 192.168.0.60 0 2301
      //
      // If a valid string is parsed, the IP will replace the content
      // of 'remoteIP' and 'proxy_state' will be == 'PROXYSTATE_PROXIED'.
      // ----------------------------------------------------------------
      if (s->proxy_state == PROXYSTATE_PARSING)                                 // source IP matched the configured "Trusted Proxy IP"
      {
#if DEBUG > 0
        if (b >=32 && b < 128)
          Serial.printf("%c", b);                                               // prints the character being parsed
#endif
        if (s->proxy_ptr < strlen(PROXY_MAGIC) && s->proxy_space_count == 0)
        {
          if (b == PROXY_MAGIC[s->proxy_ptr])                                   // if character matches with the magic string,
          {
            s->proxy_ptr++;                                                     // advance pointer for next character
          }
          else                                                                  // mismatch: change state to IDLE so this character will be processed
          {                                                                     // by the next stages (IAC parser, etc)
            s->proxy_state = PROXYSTATE_IDLE;
#if DEBUG > 0
            Serial.println("--->mismatch");
#endif
          }
        }
        else if (s->proxy_space_count == 0)                                     // looks for next space
        {
          if (b == ' ')
          {
#if DEBUG > 0
            Serial.print("[space]");
#endif
            s->proxy_space_count++;
            s->proxy_ptr = 0;
          }
        }
        else if (s->proxy_space_count == 1)                                     // IP capture phase
        {
          if (b == '.' || (b >= '0' && b <= '9'))
          {
            if (s->proxy_ptr < PROXYIPBUF_LEN)
            {
              s->proxy_ip_buf[s->proxy_ptr++] = b;
            }
          }
          else if (b != ' ' || s->proxy_ptr > 0)
          {
#if DEBUG > 0
            Serial.print("[IP_END]");
#endif
            s->proxy_ip_buf[s->proxy_ptr] = '\0';
            s->proxy_space_count = 2;                                           // last phase
          }
        }

        if (s->proxy_space_count == 2)
        {
          if (b == '\n')
          {
            s->proxy_state = PROXYSTATE_IDLE;                                   // assumes IP not parsed

            if (s->proxy_ptr > 6)                                               // minimum "x.x.x.x"
            {
              uint8_t ipbuf[4];
              if (parse_ip_string(s->proxy_ip_buf, ipbuf))
              {
                s->proxy_state = PROXYSTATE_PROXIED;                            // IP parsed!
                s->remoteIP[0] = ipbuf[0]; s->remoteIP[1] = ipbuf[1]; s->remoteIP[2] = ipbuf[2]; s->remoteIP[3] = ipbuf[3];
              }
            }

#if DEBUG > 0
            if (s->proxy_state == PROXYSTATE_PROXIED)
              Serial.printf("\r\n[PARSER] Proxied IP: %u.%u.%u.%u\r\n", s->remoteIP[0], s->remoteIP[1], s->remoteIP[2], s->remoteIP[3]);
            else
              Serial.println("\r\n[PARSER] Invalid IP string");
#endif
            rd++;
            wizWrite16(Sn_RX_RD, S_REG(s->sn), rd);
            wizWrite(Sn_CR, S_REG(s->sn), CR_RECV);
            continue;
          }
        }

        if (s->proxy_state == PROXYSTATE_PARSING)
        {
          rd++;
          wizWrite16(Sn_RX_RD, S_REG(s->sn), rd);
          wizWrite(Sn_CR, S_REG(s->sn), CR_RECV);
          continue;
        }
      }

      // ----------------------------------------------------------------
      //  RAW MODE: quasi complete parser bypass
      //
      //  RAW mode (also) prevents special commands from being handled.
      //  The RAW mode is always terminated when a client disconnects.
      // ----------------------------------------------------------------
      if (s->telnet_flags & TFLAG_RAW_MODE)
      {
        bool push_this_byte = true;

#if DEBUG > 2
        Serial.printf("[RAW] %02X\r\n", b);
#endif
        // Se abbiamo rilevato che il client parla Telnet, dobbiamo gestire l'unescaping dello 0xFF (FF FF -> FF)
        if (s->is_telnet) 
        {
          if (s->iac_state == 1)
          {
            s->iac_state = 0;
            
            // Se b NON e' FF, e' un comando IAC arrivato a tradimento (lo scartiamo).
            if (b != 0xFF)                                                      // se non e' un altro 0xFF, e' un comando IAC (lo blocca)
              push_this_byte = false;                                           // altrimenti lascia passare 0xFF (uno su due)
          }
          else if (b == 0xFF) {
            s->iac_state = 1;                                                   // trovato primo FF, aspetta il prossimo
            push_this_byte = false;                                             // scarta questo byte
          }
        }

        if (push_this_byte) {
          pushok = telnet_rx_push(s, b);
        }

        rd++;
        wizWrite16(Sn_RX_RD, S_REG(s->sn), rd);
        wizWrite(Sn_CR, S_REG(s->sn), CR_RECV);
        continue;
      }
/*
      if (s->telnet_flags & TFLAG_RAW_MODE)
      {
#if DEBUG > 2
        Serial.printf("[RAW] %02X\r\n", b);
#endif

        pushok = telnet_rx_push(s, b);

        rd++;
        wizWrite(Sn_RX_RD,   S_REG(s->sn), rd >> 8);
        wizWrite(Sn_RX_RD+1, S_REG(s->sn), rd & 0xFF);
        wizWrite(Sn_CR, S_REG(s->sn), CR_RECV);
        continue;
      }
*/
      // ---------------------------------------------------------
      //  IAC PARSER (cooked mode only, RAW mode disabled)
      // ---------------------------------------------------------
      bool consume_only = false;
      switch (s->iac_state)
      {
        case 0:
          if (b == 0xFF) {                                                    // if client sends 0xFF while RAW mode is disabled, it is a telnet client
            s->iac_state = 1;

            if (!s->is_telnet)                                                // once,
            {
              if (!(s->telnet_flags & TFLAG_FORCE_NEGO_C_M))                  // if IAC not already sent by default,
                s->send_iac = true;                                           // send it
#if DEBUG > 0
              Serial.println(F("client is_telnet\r\n"));
#endif
            }
            s->is_telnet = true;                                              // this is a telnet client
            consume_only = true;
          }
        break;

        case 1:
          if (b == 0xFA) s->iac_state = 2;
          else if (b == 0xF0) s->iac_state = 0;
          else s->iac_state = 3;
          consume_only = true;
        break;

        case 2:
          if (b == 0xFF) s->iac_state = 1;
          consume_only = true;
        break;

        case 3:
          s->iac_state = 0;
          consume_only = true;
        break;
      }

      if (consume_only)
      {
#if DEBUG > 1
        Serial.println(F("[TELNET] Consumed IAC byte"));
#endif
        rd++;
        wizWrite16(Sn_RX_RD, S_REG(s->sn), rd);
        wizWrite(Sn_CR, S_REG(s->sn), CR_RECV);
        continue;
      }

      // ---------------------------------------------------------
      //  NORMALIZATION RULES (cooked mode only)
      // ---------------------------------------------------------

      // DEL → BS
      if (b == 0x7F)
      {
#if DEBUG > 1
        Serial.println(F("[TELNET] DEL→BS"));
#endif
        b = 0x08;
      }

    // CR/LF handling
      if (b == 0x0D)
      {
#if DEBUG > 1
        Serial.println(F("[TELNET] CR detected"));
#endif

        pushok = telnet_rx_push(s, 0x0D);

//        uint8_t next = w5500_read_rx_byte(rd + 1);
        // patch della presunta causa di caratteri fantasma
        uint8_t next = 0xFF;
        if (rx > 1)
          next = w5500_read_rx_byte(s, rd + 1);

        if (next == 0x0A || next == 0x00)
        {
#if DEBUG > 1
          Serial.println(F("[TELNET] Consuming LF/NUL after CR"));
#endif
          rd++;
          rx--;
          wizWrite16(Sn_RX_RD, S_REG(s->sn), rd);
          wizWrite(Sn_CR, S_REG(s->sn), CR_RECV);
        }

        rd++;
        wizWrite16(Sn_RX_RD, S_REG(s->sn), rd);
        wizWrite(Sn_CR, S_REG(s->sn), CR_RECV);
        continue;
      }

      // ---------------------------------------------------------
      //  SPECIAL COMMANDS ([pwd]X) (only for socket 0)
      // ---------------------------------------------------------
      if (s->telnet_cmd_state < strlen(netcfg.sock0_password))
      {
        if (b == netcfg.sock0_password[s->telnet_cmd_state] && s->sn == 0)
        {
          ++s->telnet_cmd_state;
        }
        else
          s->telnet_cmd_state = 0;
      }
      else
      {
        s->telnet_cmd_state = 0;

#if DEBUG > 0
        Serial.printf("[TELNET] Special command: %c\r\n", b);
#endif

        if (b == 'E')
        {
          w5500_init();
          continue;     // era 'break' (?)
        }
        // jumps to main menu
        else if (b == 'M' && (s->telnet_flags & TFLAG_CON_BRIDGE))
        {
          if (inTheMenu == 0)
            telnetSpecialCmds = CMD_MAIN_MENU;
        }
        // Z80 hard reset
        else if (b == 'Z' && (s->telnet_flags & TFLAG_CON_BRIDGE))
          telnetSpecialCmds = CMD_Z80_RESET;
        // enable tick interrupt
        else if (b == 'I' && (s->telnet_flags & TFLAG_CON_BRIDGE))
          telnetSpecialCmds = CMD_ENABLE_TICK_INT;
        // disable tick interrupt
        else if (b == 'D' && (s->telnet_flags & TFLAG_CON_BRIDGE))
          telnetSpecialCmds = CMD_DISABLE_TICK_INT;
        // complete system reset
        else if (b == 'R' && (s->telnet_flags & TFLAG_CON_BRIDGE))
        {
          systemResetSafe();
        }
        // telnet bridge toggle
        else if (b == 'B')
        {
          s->telnet_flags ^= TFLAG_CON_BRIDGE;

          Serial.printf("[TELNET] IOS Bridge socket %u ", s->sn);
          if (s->telnet_flags & TFLAG_CON_BRIDGE)
          {
            Serial.println(F("Activated"));
            telnet_sessions[0].rx_head = telnet_sessions[0].rx_tail;          // clears any previously received character
          }
          else
            Serial.println(F("Deactivated"));
        }
//        else
//          pushok = telnet_rx_push(s, b);

        rd++;
        wizWrite16(Sn_RX_RD, S_REG(s->sn), rd);
        wizWrite(Sn_CR, S_REG(s->sn), CR_RECV);
        continue;
      }

      // ---------------------------------------------------------
      //  NORMAL CHARACTER (cooked mode)
      // ---------------------------------------------------------
      if (s->telnet_cmd_state == 0 || (s->telnet_flags & TFLAG_CON_BRIDGE))     // don't push characters to fifo if special command in progress
        pushok = telnet_rx_push(s, b);                                          // with bridging not (yet) active

      rd++;
      wizWrite16(Sn_RX_RD, S_REG(s->sn), rd);
      wizWrite(Sn_CR, S_REG(s->sn), CR_RECV);
    }
    if (safety == 0)
      Serial.printf("Safety counter RX loop Exit socket %u\r\n", s->sn);

    if (!pushok)
      Serial.printf("FIFO Full RX loop Exit socket %u\r\n", s->sn);
  }
}


// ---------------------------------------------------------
// Converts string to IP
// ---------------------------------------------------------
bool parse_ip_string(char* str, uint8_t* ip_array)
{
  char* ptr = str;
  for (uint8_t i = 0; i < 4; i++)
  {
    ip_array[i] = (uint8_t)atoi(ptr);
    ptr = strchr(ptr, '.');

    if (i < 3)
    {
      if (!ptr)
        return false;
      ptr++;
    }
  }
//  Serial.printf("parse string %s: %u.%u.%u.%u ", str, ip_array[0], ip_array[1], ip_array[2], ip_array[3]);
  return true;
}


// ---------------------------------------------------------
//  Negotiate no local echo, no go-ahead, no line buffering
//  on client. They are handled on server.
//
//  Also specifies binary transfers, preventing teraTerm
//  from adding a 0x00 after any 0x0D (teraTerm's fault)
//
//  This function is called:
//  - On any new connection if 'TFLAG_FORCE_NEGO_C_M' is set
//  - On new connections if the client is a telnet client
// ---------------------------------------------------------
static void telnet_negotiate_character_mode(TelnetSession *s)
{
//
//  IAC WILL BINARY TRANSMISSION
//
  telnet_write_char(s, 0xFF);                                                   // IAC
  telnet_write_char(s, 0xFB);                                                   // WILL
  telnet_write_char(s, 0x00);                                                   // BINARY TRANSMISSION
//
//  IAC DO BINARY TRANSMISSION
//
  telnet_write_char(s, 0xFF);                                                   // IAC
  telnet_write_char(s, 0xFD);                                                   // DO
  telnet_write_char(s, 0x00);                                                   // BINARY TRANSMISSION
//
//  IAC WILL ECHO  →  "The server will do echo"
//
  telnet_write_char(s, 0xFF);                                                   // IAC
  telnet_write_char(s, 0xFB);                                                   // WILL
  telnet_write_char(s, 0x01);                                                   // ECHO
//
//  IAC DO ECHO
//
  telnet_write_char(s, 0xFF);                                                   // IAC
  telnet_write_char(s, 0xFD);                                                   // DO
  telnet_write_char(s, 0x01);                                                   // ECHO
//
//  IAC WILL SUPPRESS-GOAHEAD
//
  telnet_write_char(s, 0xFF);                                                   // IAC
  telnet_write_char(s, 0xFB);                                                   // WILL
  telnet_write_char(s, 0x03);                                                   // SUPPRESS-GA (0x03)
//
//  IAC DO SUPPRESS-GOAHEAD
//
  telnet_write_char(s, 0xFF);                                                   // IAC
  telnet_write_char(s, 0xFD);                                                   // DO
  telnet_write_char(s, 0x03);                                                   // SUPPRESS-GA (0x03)
}


// ---------------------------------------------------------
//  RETURNS TRUE if a CLIENT is connected.
//
//  Returns FALSE if a proxy string parsing is in progress,
//  because the reported client's IP may change when parsing
//  completes.
// ---------------------------------------------------------
bool telnetClientConnected(uint8_t idx)
{
  if (!eth_ok)
    return false;

  if (idx < MAX_TELNET_SESSIONS)
    return ((telnet_sessions[idx].last_state == SOCK_ESTABLISHED || telnet_sessions[idx].last_state == SOCK_CLOSE_WAIT) &&
        telnet_sessions[idx].proxy_state != PROXYSTATE_PARSING);
  else
    return false;
}


// ---------------------------------------------------------
//  PUSH BYTE FROM CLIENT INTO LOCAL FIFO RX BUFFER
// ---------------------------------------------------------
static bool telnet_rx_push(TelnetSession *s, uint8_t b)
{
  uint16_t next = (s->rx_head + 1) % TELNET_BUF_SIZE;

  if (next != s->rx_tail)
  {
    s->rx_buf[s->rx_head] = b;
    s->rx_head = next;
#if DEBUG > 1
    Serial.printf("push_rx, socket %u, head %u, tail %u\r\n", s->sn, s->rx_head, s->rx_tail);
#endif
    return(true);
  }
#if DEBUG > 1
  Serial.printf("push_rx FAILED socket %u, head %u, tail %u\r\n", s->sn, s->rx_head, s->rx_tail);
#endif
  return(false);
}


// ---------------------------------------------------------
//  PUSH BYTE TO BE TRANSMITTED INTO LOCAL FIFO TX BUFFER
// ---------------------------------------------------------
void telnet_tx_push(uint8_t idx, uint8_t b)
{
  if (!eth_ok)
    return;

  if (idx >= MAX_TELNET_SESSIONS)
  {
    Serial.printf("[TELNET] push_tx attempted on invalid socket %u\r\n", idx);
    return;
  }

  uint16_t next = (telnet_sessions[idx].tx_head + 1) % TELNET_BUF_SIZE;

  // Se il buffer è pieno, avanza anche la tail
  if (next == telnet_sessions[idx].tx_tail)
    telnet_sessions[idx].tx_tail = (telnet_sessions[idx].tx_tail + 1) % TELNET_BUF_SIZE;

  telnet_sessions[idx].tx_buf[telnet_sessions[idx].tx_head] = b;
  telnet_sessions[idx].tx_head = next;

#if DEBUG > 1
  Serial.printf("push_tx socket %u: head %u, tail %u\r\n", telnet_sessions[idx].sn, telnet_sessions[idx].tx_head, telnet_sessions[idx].tx_tail);
#endif
}


// ---------------------------------------------------------
//  POP AND TRANSMIT BYTES FROM LOCAL FIFO TX BUFFER
//  Handles IAC 0xFF escape if a telnet client
// ---------------------------------------------------------
static void telnet_tx_pop(TelnetSession *s)
{
  while (s->tx_head != s->tx_tail)
  {
    uint8_t chunk[129];
    uint8_t count = 0;

    // raccogli un blocco contiguo
    while (s->tx_head != s->tx_tail && count < sizeof(chunk)-1)                 // -1 because we may have to encode two bytes
    {
      uint8_t b = s->tx_buf[s->tx_tail];
      s->tx_tail = (s->tx_tail + 1) % TELNET_BUF_SIZE;

      chunk[count++] = b;

      if (b == 0xFF && s->is_telnet)                                            // if it is a true telnet client and databyte is 0xFF,
        chunk[count++] = 0xFF;                                                  // adds one more 0xFF byte (IAC escape)
    }

    // invia il blocco
    telnet_write_block(s, chunk, count);
  }
}


// ---------------------------------------------------------
//  TX BLOCK (SENDS A CHUNK OF DATA OVER TELNET)
// ---------------------------------------------------------
static void telnet_write_block(TelnetSession *s, const uint8_t *buf, uint16_t len)
{
#if DEBUG > 1
  Serial.printf("[W5500] TX %u bytes socket %u\r\n", len, s->sn);
#endif

  // Check for available space
  uint16_t free = wizRead16(Sn_TX_FSR, S_REG(s->sn));                           // check available space in W5500 TX buffer

#if DEBUG > 1
  Serial.printf("[W5500] S_TX_FSR[%u] %u\r\n", s->sn, free);
#endif
  if (free < len)
    len = free;

  if (len == 0)
    return;

  s->last_rx_time = millis();                                                   // aggiorna timeout

  // Read WR pointer
  uint16_t wr = wizRead16(Sn_TX_WR, S_REG(s->sn));

  // Burst write into TX buffer
  wizBurstWrite((wr & S_TXBUF_MASK), S_TXB(s->sn), buf, len);

  // WR pointer update
  wr += len;
  wizWrite16(Sn_TX_WR, S_REG(s->sn), wr);

  // SEND
  wizWrite(Sn_CR, S_REG(s->sn), CR_SEND);
  if (wait_command_done(s->sn))
    Serial.printf("[W5500] SEND timeout on socket %u\r\n", s->sn);
}


// ---------------------------------------------------------
//  TX BYTE (SENDS A SINGLE BYTE OVER TELNET)
// ---------------------------------------------------------
static void telnet_write_char(TelnetSession *s, uint8_t b)
{
#if DEBUG > 1
  Serial.printf("[W5500] TX 0x%02X socket %u\r\n", b, s->sn);
#endif

  // Check for available space
  uint16_t free = wizRead16(Sn_TX_FSR, S_REG(s->sn));

  if (free == 0)
    return;                                                                     // buffer full

  s->last_rx_time = millis();                                                   // aggiorna timeout

  uint16_t wr = wizRead16(Sn_TX_WR, S_REG(s->sn));                              // WR pointer

  wizWrite((wr & S_TXBUF_MASK), S_TXB(s->sn), b);                               // buffer write

  wr++;                                                                         // pointer update
  wizWrite16(Sn_TX_WR, S_REG(s->sn), wr);

  wizWrite(Sn_CR, S_REG(s->sn), CR_SEND);                                       // SEND
}


// ---------------------------------------------------------
//  READ W5500 RECEIVED BYTE from W5500
// ---------------------------------------------------------
static uint8_t w5500_read_rx_byte(TelnetSession *s, uint16_t rd)
{
  // calcolare indirizzo in base a s->sn
  uint16_t addr = (rd & S_RXBUF_MASK);
  return wizRead(addr, S_RXB(s->sn));
}


// ---------------------------------------------------------
//  W5500 SOFTWARE RESET
// ---------------------------------------------------------
static bool w5500_software_reset()
{
#if DEBUG > 0
  Serial.println(F("[W5500] W5500 reset"));
#endif

  for (uint8_t i = 0; i < MAX_TELNET_SESSIONS; i++)
    w5500_disconnect(&telnet_sessions[i]);

  // Reset chip
  wizWrite(MR, 0x00, 0x80);

  uint32_t t0 = millis();
  bool success = false;
  while (millis() - t0 < 500)                                                   // Timeout di 500ms
  {
    if ((wizRead(MR, 0x00) & 0x80) == 0x00)
    {
      success = true;
      break;
    }
  }
  delay(20);

  if (!success)
  {
    Serial.println(F("[W5500] W5500 reset ERROR"));
  }
#if DEBUG > 0
  else
    Serial.println(F("[W5500] W5500 reset OK"));
#endif
  return success;
}


// ---------------------------------------------------------
//  W5500 DISCONNECT
// ---------------------------------------------------------
static void w5500_disconnect(TelnetSession *s)
{
  Serial.printf("[W5500] Disconnecting socket %u\r\n", s->sn);

  wizWrite(Sn_CR, S_REG(s->sn), CR_DISCON);                                     // Sn_CR_DISCON (FIN)
  if (wait_command_done(s->sn))
    Serial.printf("[W5500] CR_DISCON timeout on socket %u\r\n", s->sn);
  else
  {
    uint32_t t0 = millis();
    while (wizRead(Sn_SR, S_REG(s->sn)) != SOCK_CLOSED) {                       // wait for SOCK_CLOSED (1000ms timeout)
      if (millis() - t0 > 1000)
        break;
    }
  }

  wizWrite(Sn_CR, S_REG(s->sn), CR_CLOSE);                                      // Sn_CR_CLOSE anyway
  if (wait_command_done(s->sn))
    Serial.printf("[W5500] CR_CLOSE timeout on socket %u\r\n", s->sn);
}


// ---------------------------------------------------------
//  SPI LOW LEVEL
// ---------------------------------------------------------
static uint8_t wizRead(uint16_t addr, uint8_t block_shifted)
{
  ETH_SELECT();
  spiTransfer(addr >> 8);
  spiTransfer(addr & 0xFF);
//  spiTransfer((block << 3) | 0x00);
  spiTransfer(block_shifted | 0x00);
  uint8_t data = spiTransfer(0x00);
  ETH_DESELECT();
  return data;
}

static uint16_t wizRead16(uint16_t addr, uint8_t block_shifted)
{
  ETH_SELECT();
  spiTransfer(addr >> 8);
  spiTransfer(addr & 0xFF);
//  spiTransfer((block << 3) | 0x00);
  spiTransfer(block_shifted);
  uint16_t data = (uint16_t)spiTransfer(0x00) << 8;                             // MSB
  data |= spiTransfer(0x00);                                                    // LSB
  ETH_DESELECT();
  return data;
}

static void wizBurstRead(uint16_t addr, uint8_t block_shifted, uint8_t *buf, uint16_t len)
{
  ETH_SELECT();
  spiTransfer(addr >> 8);
  spiTransfer(addr & 0xFF);
  spiTransfer(block_shifted);

  for (uint16_t i = 0; i < len; i++)
    buf[i] = spiTransfer(0x00);

  ETH_DESELECT();
}

static void wizWrite(uint16_t addr, uint8_t block_shifted, uint8_t data)
{
  ETH_SELECT();
  spiTransfer(addr >> 8);
  spiTransfer(addr & 0xFF);
//  spiTransfer((block << 3) | 0x04);
  spiTransfer(block_shifted | 0x04);
  spiTransfer(data);
  ETH_DESELECT();
}

static void wizWrite16(uint16_t addr, uint8_t block_shifted, uint16_t data)
{
  ETH_SELECT();
  spiTransfer(addr >> 8);
  spiTransfer(addr & 0xFF);
//  spiTransfer((block << 3) | 0x04);
  spiTransfer(block_shifted | 0x04);
  spiTransfer(data >> 8);                                                       // MSB
  spiTransfer(data & 0xFF);                                                     // LSB
  ETH_DESELECT();
}

static void wizBurstWrite(uint16_t addr, uint8_t block_shifted, const uint8_t *buf, uint16_t len)
{
  ETH_SELECT();

  // Indirizzo di partenza
  spiTransfer(addr >> 8);
  spiTransfer(addr & 0xFF);

  // Control byte: block + write + VDM (auto-increment)
  spiTransfer(block_shifted | 0x04);
//  spiTransfer((block << 3) | 0x04);

  // Scrittura sequenziale
  for (uint16_t i = 0; i < len; i++)
    spiTransfer(buf[i]);

  ETH_DESELECT();
}

static uint8_t spiTransfer(uint8_t data)
{
  SPDR = data;
  while (!(SPSR & (1 << SPIF)));
  return SPDR;
}

static void systemResetSafe(void)
{
    cli();  // disabilita tutti gli interrupt

    // --- Spegnimento periferiche critiche ---
    // UART0
    UCSR0B = 0;
    UCSR0C = 0;

    // UART1
    UCSR1B = 0;
    UCSR1C = 0;

    // SPI
    SPCR = 0;
    SPSR = 0;

    // TWI (I2C)
    TWCR = 0;

    // Timers
    TCCR0A = 0; TCCR0B = 0;
    TCCR1A = 0; TCCR1B = 0;
    TCCR2A = 0; TCCR2B = 0;
    TCCR3A = 0; TCCR3B = 0;

    // ADC
    ADCSRA = 0;

    // Disabilita tutte le uscite (opzionale ma utile)
    DDRB = DDRC = DDRD = 0;
    PORTB = PORTC = PORTD = 0;

    // --- Attiva watchdog per forzare reset ---
    wdt_enable(WDTO_15MS);

    // Attendi il reset
    while (1) {}
}

static void telnet_debug_regs(TelnetSession *s)
{
  uint16_t rxrsr = wizRead16(Sn_RX_RSR, S_REG(s->sn));
  uint16_t rxrd  = wizRead16(Sn_RX_RD,  S_REG(s->sn));

  Serial.printf("[W5500] RX_RSR=%u  RX_RD=%u\r\n", rxrsr, rxrd);
}
