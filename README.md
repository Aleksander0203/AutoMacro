# AutoMacro

A lightweight macro recorder and playback tool for X11 Linux systems inspired by the tool TinyTask for windows. Record mouse movements, clicks, and keystrokes, then play them back automatically.

## Demo

![](/assets/Demo.gif)

## Requirements

- Linux with X11 (not compatible with Wayland)

## Dependencies

Install the required packages:

```
# Debian based distros
sudo apt install qt6-base-dev libxi-dev libxtst-dev libx11-dev
# Arch based distros
sudo pacman -S qt6-base libxi libxtst libx11
# Fedora
sudo dnf install qt6-qtbase-devel libXi-devel libXtst-devel libX11-devel
# OpenSUSE
sudo zypper install qt6-base-devel libXi-devel libXtst-devel libX11-devel
```

## Installation

Run the following commands in a terminal. Must have git installed.
```
git clone https://github.com/Aleksander0203/AutoMacro.git
cd src
qmake make.pro
make
```

This will output a file called main which you can run in the directory by doing ./main.
Make sure that it has execute permissions
