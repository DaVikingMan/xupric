#!/bin/sh

mkdir -p /usr/share/icons/hicolor/256x256/apps/
cp -f ../res/xupric.png /usr/share/icons/hicolor/256x256/apps/xupric.png
mkdir -p /usr/share/applications/
cp -f ../res/xupric.desktop /usr/share/applications/xupric.desktop

cp -n ../res/icons/xupric_star_no.png /usr/share/icons/hicolor/scalable/apps/
cp -n ../res/icons/xupric_star_yes.png /usr/share/icons/hicolor/scalable/apps/
