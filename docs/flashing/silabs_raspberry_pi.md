# Flashing Tuya ZS3L (EFR32MG21) via Raspberry Pi

This guide covers flashing custom Zigbee firmware onto Tuya ZS3L using Raspberry Pi GPIO as SWD programmer.

### What You'll Need

- Raspberry Pi (or similar with GPIO) (tested on Raspberry Pi 3B+)
- Jumper wires
- Soldering iron

### ZS3L Chip Pinout

If you have a ZS3L module, you can use the following [Tuya docs](https://developer.tuya.com/en/docs/iot/zs3l?id=K97r37j19f496) for reference. Or just follow this schema:

![PCB connections](/docs/.images/meos_4_gang_pcb.jpg)

ℹ **Note:** Sometimes there is no need to solder 3.3V/GND. You can find them on the board and use connector.
ℹ **Note:** No need to solder nRST pin. It causes connection issues.

## Step 1: Hardware Wiring

![Raspberry Pi GPIO diagram](/docs/.images/raspberry_pi_gpio_pinout.png)

Connect ZS3L to Raspberry Pi (BCM numbering):

| ZS3L Pin | Raspberry Pi | Physical Pin |
|----------|--------------|--------------|
| SWDIO    | GPIO 24      | Pin 18       |
| SWCLK    | GPIO 25      | Pin 22       |
| GND      | GND          | Pin 14       |
| VCC      | 3.3V         | Pin 1 or 17  |

⚠️ **NEVER use 5V - it will destroy the chip!**

ℹ **Note:** No need of nRST pin. It causes connection issues.

## Step 2: Compile Custom OpenOCD

Standard OpenOCD doesn't support EFM32 Series 2. Configure the [knieriem's fork](https://github.com/knieriem/openocd-efm32-series2):

```bash
sudo apt-get update
sudo apt-get install git autoconf libtool make pkg-config

git clone https://github.com/knieriem/openocd-efm32-series2.git
cd openocd-efm32-series2

./setup-openocd-src.sh
```

You will be asked to run `./build.sh`, but don't. This is not needed while using GPIO.
If you use some USB programmer, you may need to run `./build.sh` to compile the driver.

Compile the driver:

```bash
cd openocd
./configure --enable-bcm2835gpio
make
make install
```

## Step 3: OpenOCD Configuration

Create `openocd.cfg`:

```tcl
transport select swd

adapter gpio swdio 24
adapter gpio swclk 25

set CHIPNAME efr32
source [find target/efm32s2.cfg]

reset_config srst_only srst_nogate
adapter speed 1000

init
targets
halt
```

## Step 4: Start Connection

**Terminal 1** - Start OpenOCD daemon:

```bash
openocd
```

You should see something like this:
```
$ openocd
Open On-Chip Debugger 0.12.0+dev-01294-g2e60e2eca-dirty (2026-03-01-09:48)
Licensed under GNU GPL v2
For bug reports, read
	http://openocd.org/doc/doxygen/bugs.html
Warn : TMS/SWDIO moved to GPIO 8 (pin 24). Check the wiring please!
Info : BCM2835 GPIO JTAG/SWD bitbang driver
Info : clock speed 1000 kHz
Info : SWD DPIDR 0x6ba02477
Info : [efr32.cpu] Cortex-M33 r0p3 processor detected
Info : [efr32.cpu] target has 8 breakpoints, 4 watchpoints
Info : starting gdb server for efr32.cpu on 3333
Info : Listening on port 3333 for gdb connections
Warn : [efr32.cpu] target was in unknown state when halt was requested
Info : Listening on port 6666 for tcl connections
Info : Listening on port 4444 for telnet connections
Info : accepting 'telnet' connection on tcp/4444
Info : detected part: MG21A020, rev 51
Info : flash size = 768 KiB
Info : flash page size = 8192 B
Warn : Don't know EFR/EFM Gx family number, can't set MSC register. Defaulting to EF{M,R}xG22 values..
```

**Terminal 2** - Connect via Telnet:

```bash
telnet localhost 4444
```

## Step 5: Verify, Backup, Erase, and Flash

Execute in Telnet console (`>` prompt). ZS3L has 768 KiB flash (`0x000c0000`).

### Verify Connection

```shell
> halt
> targets
> flash info 0
> flash probe 0
> flash banks
> flash list
```

<details>
<summary>Example output</summary>

```shell
> halt
> targets
TargetName         Type       Endian TapName            State
--  ------------------ ---------- ------ ------------------ ------------
0* efr32.cpu          cortex_m   little efr32.cpu          halted
> flash info 0
detected part: MG21A020, rev 51
flash size = 768 KiB
flash page size = 8192 B
Don't know EFR/EFM Gx family number, can't set MSC register. Defaulting to EF{M,R}xG22 values..
#0 : efm32s2 at 0x00000000, size 0x000c0000, buswidth 0, chipwidth 0
#  0: 0x00000000 (0x2000 8kB) protected
# <redacted>
# 94: 0x000bc000 (0x2000 8kB) protected
# 95: 0x000be000 (0x2000 8kB) protected
MG21A020, rev 51
> flash probe 0
detected part: MG21A020, rev 51
flash size = 768 KiB
flash page size = 8192 B
Don't know EFR/EFM Gx family number, can't set MSC register. Defaulting to EF{M,R}xG22 values..
flash 'efm32s2' found at 0x00000000
> flash banks
#0 : efr32.flash (efm32s2) at 0x00000000, size 0x000c0000, buswidth 0, chipwidth 0
#1 : userdata.flash (efm32s2) at 0x0fe00000, size 0x00000000, buswidth 0, chipwidth 0
> flash list
{
name       efr32.flash
driver     efm32s2
base       0x00000000
size       0xc0000
bus_width  0
chip_width 0
target     efr32.cpu
}
{
name       userdata.flash
driver     efm32s2
base       0x0fe00000
size       0x0
bus_width  0
chip_width 0
target     efr32.cpu
}
```

</details>

### Backup

```shell
> dump_image flash_dump.bin 0x0 0xc0000
> dump_image userdata_dump.bin 0x0fe00000 0x400
```

### Erase and Flash
```shell
> flash erase_address 0x0 0xc0000
> flash write_image erase efr32mg21_bootloader_generic.s37
> flash write_image erase tlc_switch-1.1.2-9a4eb422.s37
> reset run
```

Device will enter pairing mode automatically. Disconnect wires and reassemble.

