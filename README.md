# hidscan

Scan for HID devices, and print out their usages.

This utility will show you what kind of hid devices are present on your system.


## Usage

```
$ ./hidscan 
CH PRODUCTS CH THROTTLE QUADRANT
	Generic Desktop/Joystick,Pointer
Razer Razer DeathAdder V2
	Generic Desktop/Mouse,Pointer
Razer Razer DeathAdder V2
	Generic Desktop/Keyboard
	Consumer/
	Generic Desktop/System Control,Undefined,Undefined
Razer Razer DeathAdder V2
	Generic Desktop/Keyboard
Razer Razer DeathAdder V2
	Lighting And Illumination/LampArray CA,LampArrayAttributesReport,LampAttributesRequestReport,LampAttributesResponseReport,Reserved,Reserved,Reserved
GSAS Inc Turbo LEDz ODO
	Vendor-defined/
GSAS Inc Turbo LEDz 810c
	Vendor-defined/
AsusTek Computer Inc. AURA LED Controller
	Vendor-defined/
NUVOTON LED Dongle
	Generic Desktop/Undefined
	Vendor-defined/
Realtek HID Device
	Vendor-defined/
Ducky AKKO 3087
	Generic Desktop/Keyboard
Ducky AKKO 3087
	Generic Desktop/Keyboard,Wireless Radio Controls
	Consumer/
	Vendor-defined/
TinyUSB TinyUSB Device
	Generic Desktop/Keyboard,Mouse,Pointer
```

## Building

The hidscan utility depends on the hidapi library. On debian-like systems, do:
```
$ sudo apt install libhidapi-dev
```

To build hidcan, use:
```
$ make
```

## Arguments

### --verbose / -v

Print out extra information on stderr.

## Alternative

This was created as an alternative to `hidapitester --list-usages` because that tool will only output usage codes, not usage names.

Also, hidscan will nicely group functions of a single device with many raw hid device files.

