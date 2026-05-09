# Z80-MBC2-V

**UNDER CONSTRUCTION**

The **Z80-MBC2-V** is a variant of the popular Z80-MBC2 (Multi Boot Computer 2) by Just4Fun.

## Main Features
- Z80 CMOS running up to 10MHz
- ATMega1284P for I/O
- Ethernet (WizNet W5500) for remote (telnet) access and/or running CBBS software
- Supercap-backed 512KB RAM (some of which used for a CP/M 3.0 Ramdisk)
- Hardware watchdog
- 3,3V I2C interface for small (128x64) OLED display
- 8-24V power supply (as an alternative to USB power)
- TTL serial I/O (as an alternative to USB data)
- RC2014 expansion bus

## Repository Structure
- `/Hardware`: Schematics (PDF) and Gerber files for PCB manufacturing
- `/Firmware`: Source code for the ATMega1284P MCU
- `/Software`: Disk Images, Utilities
- `/Docs`    : Technical documentation

## 🌐 Project Page
[https://probosci.de/Z80mbc](https://probosci.de/Z80mbc)

---
*Based on the original Z80-MBC2 project*

