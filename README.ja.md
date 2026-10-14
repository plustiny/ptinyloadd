# ptiny.load
=======

- [English](./README.md)
- [日本語](./README.ja.md)

## 概要

pyLoad に着想を得た、シンプルで軽量なサーバーサイドダウンローダーです。

## インストール

### サーバー (Linux)

```bash
sudo apt install libmicrohttpd-dev libjansson-dev zlib1g-dev

git clone https://github.com/plustiny/ptinyloadd.git
cd ptinyloadd
make
sudo make install

sudo vi /usr/local/etc/ptinyloadd/ptinyloadd.conf  # PORT, NAME, PASSWD を変更
sudo ln -s /etc/letsencrypt/live/YOUR\_SERVER/privkey.pem /usr/local/etc/ptinyloadd/privkey.pem
sudo ln -s /etc/letsencrypt/live/YOUR\_SERVER/fullchain.pem /usr/local/etc/ptinyloadd/fullchain.pem
```

### クライアント (Android)

```bash
git clone  https://github.com/plustiny/ptinyloadd.git
cd ptiny.load
./gradlew build
adb install app/build/outputs/apk/debug/app-debug.apk
```

##使い方
Android のアプリに URL を送ると、ダウンロードされます。

##サポート
このプロジェクトを応援していただける場合は、iHerbでのお買い物の際に以下のRewards Codeをご利用ください。
**Rewards Code:** GEM796
また、Bitcoinでのご支援も受け付けています。以下のBTCアドレス宛にお送りください。
`bitcoin:bc1qjjlzy93cmfxpswg2mrzf84v87wchpaagk54nqm`
