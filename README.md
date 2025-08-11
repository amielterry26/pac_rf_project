# PAC-RF Project

## Build Instructions

```bash
mkdir build && cd build
cmake ..
make
./bin/pac_rf_exec --bitwidth 8
```

## Description

This is a modular C refactor of the PAC-RF system. It supports bit-width parsing (4/8/16-bit) and is ready for future modules including overflow handling and GUI integration.
