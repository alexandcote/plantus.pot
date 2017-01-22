# Plantus-Pot

Install GCC for ARM
```
brew cask install gcc-arm-embedded
```

Installing mbed-cli
```
pip install mbed-cli
```

Clonning the repository
```
mbed import git@github.com:alexandcote/plantus-pot.git plantus-pot
```

Compile the project
```
mbed compile -m LPC1768 -t GCC_ARM
```

Run the script to copy the binary file on the MBED
```
./mbed_copy BUILD/LPC1768/GCC_ARM/plantus-pot.bin /Volumes/MBED
```
