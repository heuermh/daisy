# patch_init_daisysp (libDaisy)

[DaisySP](https://github.com/electro-smith/DaisySP) sketch for a Daisy Patch SM (patch.Init()), built with libDaisy.
It plays the same random notes as [`../patch_init_chuck`](../patch_init_chuck), with a DaisySP `Oscillator` and `Adsr`
instead of ChucK.


## Memory

* The program is ~100 KB. It runs from SRAM via the Daisy bootloader (`APP_TYPE = BOOT_SRAM`): the bootloader
  copies it from QSPI flash to SRAM at boot. This keeps the bootloader installed for the ChucK sketches, so
  you can switch between them with `make program-dfu` alone.


## Build

Requires the [Daisy toolchain](https://docs.daisy.audio/tutorials/cpp-dev-env/#1-install-the-toolchain) (`arm-none-eabi-gcc`, `dfu-util`).

Override the locations with `make LIBDAISY_DIR=... DAISYSP_DIR=...`, and the toolchain with `GCC_PATH=...`, if necessary.

```bash
# on osx via homebrew
brew install make gcc-arm-embedded dfu-util open-ocd

# once: clone and build libDaisy and DaisySP (ignored by git)
git clone --recurse-submodules https://github.com/electro-smith/libDaisy ../libDaisy
make -C ../libDaisy

git clone https://github.com/electro-smith/DaisySP ../DaisySP
make -C ../DaisySP

# build sketch to .hex and .bin files
make
```


## Flash

The Daisy bootloader must be installed once, see [`../patch_init_chuck`](../patch_init_chuck/README.md#flash).
Then, within the bootloader's 2 s grace period after reset (LED pulsing):

```bash
make program-dfu
```

USB serial logging works as described in [`../patch_init_chuck`](../patch_init_chuck/README.md#usb-serial).


## Usage

* B7 plays a random note, MIDI notes 48 to 72
* B8 switches the output between hard and soft clip; the C1 LED shows hard clip
* USB serial prints audio callback CPU load once per second
