#!/bin/sh
inkscape --without-gui --export-png=../android-project/res/drawable-mdpi/ic_launcher.png   -w 48  -h 48  icon.svg
inkscape --without-gui --export-png=../android-project/res/drawable-hdpi/ic_launcher.png   -w 72  -h 72  icon.svg
inkscape --without-gui --export-png=../android-project/res/drawable-xhdpi/ic_launcher.png  -w 96  -h 96  icon.svg
inkscape --without-gui --export-png=../android-project/res/drawable-xxhdpi/ic_launcher.png -w 144 -h 144 icon.svg
