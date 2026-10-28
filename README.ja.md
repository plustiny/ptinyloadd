# ptiny.load
=======

- [English](./README.md)
- [日本語](./README.ja.md)

## 概要

pyLoad に着想を得た、シンプルで軽量なサーバーサイドダウンローダーです。

## ソースコード

Server(linux)<br>
[https://github.com/plustiny/ptinyloadd.git](https://github.com/plustiny/ptinyloadd.git)<br>
Client(Android)<br>
[https://github.com/plustiny/ptiny.load.git](https://github.com/plustiny/ptiny.load.git)<br>

## インストール

### サーバー (Linux)

```bash
sudo apt install libmicrohttpd-dev libjansson-dev zlib1g-dev libwrap0-dev

git clone  https://github.com/plustiny/ptinyloadd.git
cd ptinyloadd
vi ptinyloadd.service # config LoadCredential
vi ptinyloadd.conf    # config PORT, NAME, PASSWORD
make
sudo make install
sudo make service
```

### クライアント (Android)

```bash
git clone  https://github.com/plustiny/ptiny.load.git
cd ptiny.load
./gradlew build
adb install app/build/outputs/apk/debug/app-debug.apk
```

##使い方
Android のアプリに URL を送ると、ダウンロードされます。

##サポート
このプロジェクトを応援していただける場合は、iHerbでのお買い物の際に以下のRewards Codeをご利用ください。
Rewards Code: **GEM796**
また、Bitcoinでのご支援も受け付けています。以下のBTCアドレス宛にお送りください。
`bitcoin:bc1qjjlzy93cmfxpswg2mrzf84v87wchpaagk54nqm`
