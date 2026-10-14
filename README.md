# ptiny.load
=======

- [English](./README.md)
- [日本語](./README.ja.md)

## Overview

A simple, lightweight server-side downloader inspired by pyLoad.<br>

## Installation

### Server (Linux)

```bash
sudo apt install libmicrohttpd-dev libjansson-dev zlib1g-dev

git clone  https://github.com/plustiny/ptinyloadd.git
cd ptinyloadd
make
sudo make install

sudo vi /usr/loacl/etc/ptinyloadd/ptinyloadd.conf # change PORT, NAME, PASSWd
sudo ln -s /etc/letsencrypt/live/YOUR_SERVER/privkey.pem /usr/local/etc/ptinyloadd/privkey.pem
sudo ln -s /etc/letsencrypt/live/YOUR_SERVER/fullchain.pem /usr/local/etc/ptinyloadd/fullchain.pem
```

### Client (Android)

```bash
git clone  https://github.com/plustiny/ptinyloadd.git
cd ptiny.load
./gradlew build
adb install app/build/outputs/apk/debug/app-debug.apk
```

## Usage
Send a URL to the app on Android, and it will download it.<br>

## Donate
If you'd like to support this project, 
please use my [iHerb](https://iherb.com/) Rewards Code at checkout.
**Rewards Code:** GEM796

If you’d like to support this project, please send Bitcoin to:
bitcoin:bc1qjjlzy93cmfxpswg2mrzf84v87wchpaagk54nqm

