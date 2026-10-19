# ptiny.load
=======

- [English](./README.md)
- [日本語](./README.ja.md)

## Overview

A simple, lightweight server-side downloader inspired by pyLoad.<br>

## Source Code

Server(linux)<br>
[https://github.com/plustiny/ptinyloadd.git](https://github.com/plustiny/ptinyloadd.git)<br>
Client(Android)<br>
[https://github.com/plustiny/ptiny.load.git](https://github.com/plustiny/ptiny.load.git)<br>

## Installation

### Server (Linux)

```bash
sudo apt install libmicrohttpd-dev libjansson-dev zlib1g-dev libwrap0-dev

git clone  https://github.com/plustiny/ptinyloadd.git
cd ptinyloadd
vi ptinyloadd.conf # change PORT, NAME, PASSWD
make
sudo make install

sudo ln -s /etc/letsencrypt/live/(YOUR_SERVER)/privkey.pem /usr/local/etc/ptinyloadd/privkey.pem
sudo ln -s /etc/letsencrypt/live/(YOUR_SERVER)/fullchain.pem /usr/local/etc/ptinyloadd/fullchain.pem
sudo make service
```

### Client (Android)

```bash
git clone  https://github.com/plustiny/ptiny.load.git
cd ptiny.load
./gradlew build
adb install app/build/outputs/apk/debug/app-debug.apk
```

## Usage
Send a URL to the app on Android, and it will download it.<br>

## Donate
If you'd like to support this project, 
please use my [iHerb](https://iherb.com/) Rewards Code at checkout.
Rewards Code: **GEM796**

If you’d like to support this project, please send Bitcoin to:
bitcoin:bc1qjjlzy93cmfxpswg2mrzf84v87wchpaagk54nqm

