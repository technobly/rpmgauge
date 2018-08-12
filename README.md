# RPM Gauge
A Car Heads-up RPM Gauge built with Carloop, Particle Photon and Neopixel sticks.

https://youtu.be/N_5b5xuIu-s

## Summary

- Uses Mode 1 PID 0x0c for RPM, should work on most vehicles!
- Polls and displays faster than my cluster's RPM gauge!!
- Brighter Indicators at 1k,2k,3k,4k,5k,6k,7k
- Needle changes to Magenta when passing SHIFT_POINT_RPM

## TODO
- Add a physical brightness trimpot to the setup to control the brightness (Day/Night needs a big difference in output)
- On/off switch for the OBD-II port so I don't have to unplug it
- Make all wires black
- Mount display possibly using blue tack, and mount it a bit closer to the windshield so it's also lower on the "heads-up" display.
- Add a pleasant sounding piezo for the shift point?

## Compile
```
$ cd rpmgauge
$ particle compile photon . --target 0.8.0-rc.9 --saveTo firmware.bin && particle flash --usb firmware.bin
// feel free to change the above target firmware version! ...or flash OTA instead of over USB.
```

## License

MIT
