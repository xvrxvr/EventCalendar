# EventCalendar
Event Calendar project

This project coinsists of dedicated hardware device (with ESP32 mcu, LCD, fingerprint sensor and bunch of solenoids which used as control lockes for set of small doors)
Each door closes separated box with some gift.
User should solve some challenge to open door.

## Contents of project

Directories content:

* 3D contains 3D model for panel around LCD
* sch - Schema. For now PCB version is outdated (some schema modification was done after PCB was provided). Also 2 schems exists:
  * **ev_cal** - Calendar itself
  * **vboost** - Voltage boost for solenoids (it pluged in instead of L1)
* soft - Software. 3 directory included:
  * expr_quest - Prototype (in Python) for evaluation challenge
  * firmware - Firmware (VS Code + IDF plugin)
  * sg - Sign generator scripts
  * tst - Hardware test (VS Code + IDF plugin)
* spice - Spice mode for Voltage booster


