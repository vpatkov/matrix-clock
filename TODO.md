Remake from scratch:

- Don't remake wireless modules (`pc-link` and `outdoor`), they are ok.
  Some cosmetic fixes maybe.
- Single board, use small SMD components. For ISP programming use pogo pads
  (like in the wireless modules).
- Try to put all the sensors on a single bus (I2C or SPI).
- Don't use breakout boards. Maybe except NRF24L01+, but solder it directly
  (like in the wireless modules).
- Don't use 32k crystal, use a single 8M XO oscillator.
- Maybe serial interface (CP2102N) for changing settings, debugging, etc.
- Maybe emergency battery to keep the RTC. Maybe an RTC chip.
- Log weather and send it to the PC.
- CO2 sensor?
- Buzzer
- Fully enclose in a 3D printed case.
