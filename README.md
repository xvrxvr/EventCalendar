# EventCalendar
Event Calendar Project

This project is a custom hardware device (with an ESP32 microcontroller, an LCD, a fingerprint sensor, and a bunch of solenoids that are used as control locks for a set of small doors)
Each door closes a separate box with some kind of gift.
The user must solve some kind of puzzle to open the door.

## Project Contents

Directory Contents:

* 3D contains a 3D model for the panel around the LCD
* sch - Schematic. The PCB version is outdated at this point (some schematic changes were made after the PCB was provided). There are also 2 schematics:
  * **ev_cal** - The calendar itself
  * **vboost** - Voltage boost for the solenoids (it is connected instead of L1)
* soft - Software. 6 directories included:
  * expr_quest - Prototype (in Python) for benchmark testing
  * firmware - Firmware (VS Code + IDF plugin)
  * sg - Character generator scripts
  * tst - Hardware test (VS Code + IDF plugin)
  * bloader - OTA loader (OTA is also supported in the main software)
  * ext_logger - External logger client. The external logging system is currently under development. It allows forwarding all internal UART messages to a remote TCP server. This system (with OTA capability) allows creating and debugging new software versions without special software hardware.
* spice - Spice mode for the voltage amplifier


