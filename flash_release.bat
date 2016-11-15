@rem This batch file runs ST-Link utility to flash the Release firmware to the device
"C:\Program Files (x86)\STMicroelectronics\STM32 ST-LINK Utility\ST-LINK Utility\ST-LINK_CLI.exe" -P bin\release\F7Discovery.hex -Rst
-pause