# OWL
## Overview
The OWL (OneWire Locator) is a 1-Wire addresses reader, to be used for identifying modules of the ECAL-P detector in LUXE.

## Build & run
Assuming `esp-idf` is installed and configured, run
```bash
idf.py set-target esp32s3    # chip used in OWL
idf.py menuconfig            # reconfigure the firmware (OWL component)
idf.py build                 # build the project
idf.py flash monitor         # flash and preview logs, if board is connected
```


