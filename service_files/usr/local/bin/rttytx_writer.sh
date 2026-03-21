#!/bin/bash

while true; do
    # Véletlen várakozás 0-600 másodperc között
    sleep_time=$((RANDOM % 61))
    sleep $sleep_time

    # Idő és gépnév hozzáfűzése a fájlhoz
#   printf "RYRYRY\n\r%s %s\n\r" "$(date '+%Y-%m-%d %H:%M:%S')" "$(hostname)" >> /tmp/rttytx
#   printf "RYRYRY %s %s\n" "$(date '+%Y-%m-%d %H:%M:%S')" "$(hostname)" >> /tmp/rttytx
#    printf "RYRYRY\n\r%s %s\n\r" "$(date '+%Y-%m-%d %H:%M:%S')" "$(hostname) " >> /tmp/rttytx
#    printf "\x02RYRYRY\n\r%s %s\n\r\x04" "$(date '+%Y-%m-%d %H:%M:%S')" "$(hostname)" >> /tmp/rttytx
#    printf "RYRYRY\n\r%s %s\n\r" "$(date '+%Y-%m-%d %H:%M:%S')" "$(hostname)" >> /tmp/rttytx
printf "\x02RYRYRY\n\r%s %s %d\n\r\x04" "$(date '+%Y-%m-%d %H:%M:%S')" "$(hostname)" "$sleep_time" >> /tmp/rttytx





done
