B31DG_A2/
├── .vscode/                 # VS Code settings
├── build/                   # ESP-IDF build output
├── main/
│   ├── include/
│   │   ├── monitor.h        # Monitor function declarations
│   │   ├── pins.h           # Pin definitions and input/output interface
│   │   ├── tasks.h          # Task function declarations
│   │   └── workkernel.h     # WorkKernel interface
│   ├── CMakeLists.txt       # Component build file for main/
│   ├── main.c               # app_main(), cyclic executive, frame scheduling
│   ├── monitor.c            # Runtime monitoring, deadline/miss reporting
│   ├── pins.c               # GPIO, PCNT, sync detection, sporadic ISR
│   ├── tasks.c              # Task implementations A, B, AGG, C, D, S
│   └── workkernel.c         # WorkKernel implementation
├── .clangd                  # clangd configuration
├── .gitignore               # Git ignore rules
├── CMakeLists.txt           # Top-level ESP-IDF CMake file
├── README.md                # Project documentation
├── sdkconfig                # Current ESP-IDF configuration
├── sdkconfig.ci             # CI/default config
├── sdkconfig.old            # Backup/previous config
├── wokwi.toml               # Wokwi simulation config
├── wokwi.vcd                # Wokwi waveform output   -- the simulation does not work fully so files can be ignored
└── diagram.json             # Wokwi circuit diagram 