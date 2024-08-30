
# ESPEasy custom build

This is a quick start guide to using the ESPEasy custom build fork from [https://github.com/hansrune/ESPEasy-custom](https://github.com/hansrune/ESPEasy-custom)


## Prereqs

You will need:

- A `bash` shell environment or similar (Linux, MacOS, WSL does not really matter)
- python 3.8 or later

## Install tools

```bash
python -V                                                   # assess your python version 
mkdir -p ~/venv                                             # any top level dir for virtual environments will do
python3 -m venv ~/venv/platformio                           # set up a new python venv for platformio
source ~/venv/platformio/bin/activate                       # activate this environment
pip install pip --upgrade                                   # upgrade pip install first 
pip install esptool                                         # you will need esptool for uploads to devices
pip install platformio                                      # this will take some time ,,,, 
sudo apt install tio                                        # optionally install tio or serial console IO from ESP devices
```

## Prepare code from git

```bash
cd && cd git                                                # go to where  you use your local git repositories
git clone https://github.com/hansrune/ESPEasy-custom.git    # clone this ESPEasy fork
cd ESPEasy-custom/  
git checkout builds/custom/mega-20240822-1                  # check out latest build
touch ../secrets.h                                          # see src/include/../Custom.h if you want to set up your own 
git status                                                  # verify that you are on the expected branch
``


## Build 

```bash
pio run --environment public_IR_ESP32c3_4M316k_LittleFS_CDC # this will take a long time as it also downloads all tools and libraries needed
```

## Upload

Connect the ESP device to your system. In this case it is available as `/dev/ttyACM0`.

The date in the filename stems from `src/include/../Custom.h`. Change as needed.

For a first / factory install:

```bash
esptool.py --port /dev/ttyACM0 write_flash 0x0 build_output/bin/ESP_Easy_mega_20240830_public_IR_ESP32c3_4M316k_LittleFS_CDC.factory.bin
```

.. and for later updates

```bash
esptool.py --port /dev/ttyACM0 write_flash 0x10000 build_output/bin/ESP_Easy_mega_20240830_public_IR_ESP32c3_4M316k_LittleFS_CDC.bin
```

## Connect to serial port

You should check that the code loads from using a serial console. For example

```bash
tio /dev/ttyACM0
```

(use Ctrl+t q to quit)