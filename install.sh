#!/bin/bash
# install.sh – telepítő szkript az rtty8-hoz

set -e  # hiba esetén megáll

# Ellenőrizzük, hogy van-e librpitx könyvtár
if [ ! -d "librpitx" ]; then
    echo "librpitx könyvtár nem található, letöltés..."
    git clone https://github.com/F5OEO/librpitx.git
else
    echo "librpitx már létezik, frissítés..."
    cd librpitx
    git pull
    cd ..
fi

# Fordítás
echo "Fordítás..."
make clean
make

# Telepítés (opcionális)
read -p "Telepítsük a rendszerbe? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    sudo make install
    echo "Telepítve: /usr/local/bin/rtty8"
else
    echo "A bináris a jelenlegi könyvtárban: ./rtty8"
fi

echo "Kész."
