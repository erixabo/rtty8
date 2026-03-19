# rtty8 – RTTY adó Raspberry Pi-hez

Ez a program RTTY jelet sugároz a Raspberry Pi GPIO 4-es lábára kötött antennán keresztül.
A program a librpitx könyvtárat használja, amelyet a csomag automatikusan letölt és lefordít.

## Használat

1. Futtasd az `install.sh` scriptet (letölti a librpitx-et és lefordítja az rtty8-at):
   ```bash
   chmod +x install.sh
   ./install.sh
