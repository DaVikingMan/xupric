#!/bin/sh
# Note : These files are only to be ran inside the current folder, if it's ran outside it, it wouldn't work

mkdir -p ~/.config/xupric/styles/

cd ../
cp -n xupric.conf ~/.config/xupric/
cp -n src/css/main.css ~/.config/xupric/styles/
cp -n src/css/dark_mode.css ~/.config/xupric/styles/
cp -n src/css/scrollbar.css ~/.config/xupric/styles/
