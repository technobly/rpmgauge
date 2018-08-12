# RPM Gauge
A Car Heads-up RPM Gauge built with Carloop, Particle Photon and Neopixel sticks.

## Compile
```
$ cd rpmgauge
$ particle compile photon . --target 0.8.0-rc.9 --saveTo firmware.bin && particle flash --usb firmware.bin
// feel free to change the above target firmware version! ...or flash OTA instead of over USB.
```

## License

MIT
