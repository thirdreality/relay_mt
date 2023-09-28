# `Third Reality Plug`

## Circuit Schematic User Guide
- ESP-01S Relay socket is used with TRWB9 relay module plugged in
- TRWB9 relay module
<img src="./docs/TRWB9.jpg" style="zoom:23%;" />
- ESP-01S board
<img src="./docs/ESP01S.jpg" style="zoom:23%;" />
- TRWB9 relay module works with ESP-01S
<img src="./docs/TRWB9_with_ESP01S.jpg" style="zoom:23%;" />
- Please take [guide](./docs/TRWB9_Module_Specification_V1.1.pdf) for more detail.

## Initial setup

The following steps in this document were validated on Ubuntu 18.04/20.04 and
Mac OS.

- Install dependencies as specified in the **connectedhomeip** repository:
  [Building Matter](https://github.com/project-chip/connectedhomeip/blob/master/docs/guides/BUILDING.md).
- Clone and initialize the **connectedhomeip** repo

  ```
  git clone https://github.com/thirdreality/tr_relay.git
  cd tr-relay
  git submodule update --init --recursive
  source ./scripts/activate.sh
  ```
- Setup build environment for `Bouffalo Lab` SoC (Skip this step Bouffalo Lab`SDK already installed) Run`setup.sh`to install`Bouffalo Lab` SDK to /opt/bouffalolab_sdk

  ```
  cd third_party/bouffalolab/repo
  sudo bash scripts/setup.sh
  ```
- Please execute following command to export `BOUFFALOLAB_SDK_ROOT` before
  building.

  ```
  export BOUFFALOLAB_SDK_ROOT=/opt/bouffalolab_sdk
  ```
- Build Third Reality Plug

  ```
  ./scripts/build/build_examples.py --target thirdreality-relay-light build
  ```

## Download image

- Using script `*.flash.py`.

  After building gets done, python script `thirdreality-relay.flash.py` will generate under build output folder `out/thirdreality-relay-light`.

  > Note 1, `*.flash.py` should be ran under Matter build environment; if
  > python module `bflb_iot_tool` is not found, please try to do
  > `source scripts/bootstrap.sh` or install as
  > `pip3 install bflb-iot-tool`.
  > Note 2, different build options will
  > generate different output folder.
  >

  Download operation steps as below, please check `help` option of script for
  more detail.

  - Connect the board to your build machine
  - Put the board to the download mode:

    - Hold the **BOOT** button when Power-On, then release the button.
  - Type following command for image download. Please set serial port
    accordingly, here we use /dev/ttyACM0 as a serial port example.

    - `thirdreality-relay` without additional build options

      ```shell
      ./out/thirdreality-relay-light/thirdreality-relay.flash.py --port /dev/ttyACM0
      ```
    - To wipe out flash and download image, please append `--erase` to the
      above command.

      ```shell
      ./out/thirdreality-relay-light/thirdreality-relay.flash.py --port /dev/ttyACM0 --erase
      ```
- Using `Bouffalo Lab` GUI flash tool `BLDevCube`, please download on
  [this page](https://dev.bouffalolab.com/download).

  - Hold BOOT pin and reset chip, put the board in download mode.
  - Select `DTS` file;
  - Select Partition Table under
    `./out/thirdreality-relay-light/partition_cfg_4M.toml`
  - Select Firmware Bin;
  - Select Chip Erase if need;
  - Choose Target COM port.
  - Then click Create & Download.

## Run the example

- You can open the serial console. For example, if the device is at
  `/dev/ttyACM0` with UART baudrate 2000000 built:

  ```shell
  picocom -b 2000000 /dev/ttyACM0
  ```

  - To do factory reset, press **Reset** button 3 Times.

## User Manual
- Please take [guide](./docs/TRWB9_Relay_Module_User_Guide.pdf) for more detail.
