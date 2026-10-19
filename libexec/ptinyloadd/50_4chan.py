#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# @name			4chan
# @description	download images from 4chan
# @match        "https?://[w.]*boards.4chan.org/"
"""
パターン
File: <a title="532db43135c380dc7b5db9af5fb3dc79f646046b83956a3af7d75ed2e014bd95.jpg" href="//is2.4chan.org/a/1714134870397234.jpg" target="_blank">532db43135c380dc7b5db9af5(...).jpg</a> (1.27 MB, 2544x1668)</div>
<a class="fileThumb" href="//is2.4chan.org/a/1714134870397234.jpg" target="_blank"><img src="//i.4cdn.org/a/1714134870397234s.jpg" alt="1.27 MB" data-md5="etmp+7/M+dKs8TmqDtQ0FQ==" style="height: 81px; width: 125px;" loading="lazy"><div data-tip data-tip-cb="mShowFull" class="mFileInfo mobile">1.27 MB JPG</div></a></div>

ファイルサーバはi.4cdn.orgの他にis2.4chan.orgってのがあるようだ

File: <a href="//is2.4chan.org/a/1714110604687580.jpg" target="_blank">5centemiters.jpg</a> (157 KB, 578x640)</div><a class="fileThumb" href="//is2.4chan.org/a/1714110604687580.jpg" target="_blank"><img src="//i.4cdn.org/a/1714110604687580s.jpg" alt="157 KB" data-md5="l/C+wxg4aeYo4ZaUPm/Idw==" style="height: 250px; width: 225px;" loading="lazy"><div data-tip data-tip-cb="mShowFull" class="mFileInfo mobile">157 KB JPG</div></a>
titleがない

File: <a title="[sound=https%3A%2F%2Fa.pomf.cat%2Fyoqalf.m4a].png" href="//is2.4chan.org/a/1714306999890194.png" target="_blank">[sound=https%3A%2F%2Fa.po(...).png</a> (178 KB, 500x500)</div>

"""
import sys
import shutil
import re
import os
import base64
import subprocess
import time
import urllib.request
import urllib.error
from urllib.parse import unquote
from html.parser import HTMLParser

TIMEOUT = 60

def check_command(cmd):
    if shutil.which(cmd) is None:
        print(f"{cmd} not found.", file=sys.stderr)
        sys.exit(1)

def sanitize_str(s):
    s = s.strip()
    s = re.sub(r'[\\|/|:|?|"|<|>|\|]', '-', s)
    s = re.sub(r'[\t|\n|\r| ]', '_', s)
    return s

def normalize_url(url):
    url = url.strip()
    if not url:
        return ""
    if url.startswith("//"):
        url = "https:" + url
    if not url.startswith(("http://", "https://")):
        url = "http://" + url
    return unquote(url)

def get_html(url):
    print(f"downloading {url} .")
    html = ""
    urls = []
    urls.append(url)
    urls.append(re.sub(r'^(https?://)([^/]+)', r'\1desuarchive.org', url))
    for u in urls:
        try:
            req = urllib.request.Request(
                u,
                headers={'User-Agent': 'Mozilla/5.0'}
            )
            with urllib.request.urlopen(req, timeout=30) as res:
                html = res.read().decode('utf-8')
                break
        except (urllib.error.URLError, UnicodeDecodeError, IndexError) as e:
            print(e, file=sys.stderr)
    if not html:
        print("download failed.", file=sys.stderr)
        sys.exit(1)
    return html

def parse_title(url, html):
    arr = []
    m = re.match(r'https?://[^/]+4chan.org/([^/]+)/[^/]+/([^/]+)', url)
    if m:
        arr.append(m.group(1)) #board: a,pol,...
        arr.append(m.group(2)) #thread: 12345678
    m = re.search('<span class="subject">([^<]+)</span>', html)
    if m:
        arr.append(m.group(1))
    else:
        m = re.search('<meta name="description" content="(.*?)"',html)
        if m:
            arr.append(m.group(1).split(" - ")[0])
    return "_".join(arr)

def get_dl_dir(url, title):
    print("parse title.")
    dl_dir = os.getenv("DL_DIR")
    if not dl_dir:
        dl_dir = "/var/tmp"
    dl_dir = os.path.join(dl_dir, title)
    dl_dir = dl_dir[0:255]
    dl_dir = os.path.join(dl_dir,"")
    if not os.path.exists(dl_dir):
        os.makedirs(dl_dir)
    return dl_dir

def save_page(html, title, dl_dir):
    with open(os.path.join(dl_dir, title +".html"),"w") as file:
        file.write(html)
        print(f"save thread log file.")

def parse_tag(html):
    print("parsing a img tags.")
    links = set()
    for m in re.finditer(r'File: <a href="([^"]*)"[^>]*>([^<]*)</a>',html):
        url  = m.group(1)
        name = m.group(2)
        url = normalize_url(url)
        print(f"({url}, {name})")
        links.add((url, name))
    return links

def parse_dtag(html):
    print("parsing download tags.") # desuarchive.org
    links = set()
    for m in re.finditer(r'<a href="([^"]*)" download="([^"]*)">',html):
        url  = m.group(1)
        name = m.group(2)
        url = normalize_url(url)
        print(f"({url}, {name})")
        links.add((url, name))
    return links

def parse_mp3(html):
    print("extract mp3.")   # /a/ sound thread
    links = set()
    for m in re.finditer(r'File: <a title="([^"]*)"\s+href="([^"]*)"',html):
        title = m.group(1) # '[sound=https%3A%2F%2Fa.pomf.cat%2Fyoqalf.m4a].png'
        url   = m.group(2) # '//is2.4chan.org/a/1714306999890194.png'
        url = normalize_url(url)
        if not title.startswith("["):
            name = title
            url  = normalize_url(url)
            print(f"({url}, {name})")
            links.add((url, name))
        else:
            n = re.match(r'([^\[]*)(.+)',title)
            prefix = n.group(1).replace(' ','_')
            sounds = n.group(2)
            if not prefix:
                prefix = ""
            else:
                prefix = prefix + "_"
            name = prefix + os.path.basename(url)
            print(f"({url}, {name})")
            links.add((url, name))
            for l in re.finditer(r'\[([^=]+)=([^\]]+)\]', sounds):
                tag = l.group(1)
                url = l.group(2)
                if("sound" in tag):
                    url = normalize_url(url)
                    name = prefix + os.path.basename(url)
                    print(f"({url}, {name})")
                    links.add((url, name))
                if("catbox" in tag):
                    url = "https://files.catbox.moe/" + url
                    url = normalize_url(url)
                    name = prefix + os.path.basename(url)
                    print(f"({url}, {name})")
                    links.add((url, name))
    return links

def parse_hidden(html):
    print("base64 hidden url")  # /ic/ artbook
    b64pat = re.compile(r"([A-Za-z0-9+/_-]{25,}={0,2})")
    s = html
    s = re.sub(r'<wbr>', '', s)
    s = re.sub(r'<br>', '\n', s)
    s = re.sub(r'<a\b[^>]*>.*?</a>', '', s, flags=re.IGNORECASE | re.DOTALL)
    s = re.sub(r'<[^>]*>', '', s)
    links = set()
    for m in b64pat.finditer(s):
        b = m.group(1)
        print(b)
        try:
            url = base64.b64decode(b, validate=True).decode("utf-8")
            if b64pat.match(url):
                url = base64.b64decode(url, validate=True).decode("utf-8")
        except Exception:
            continue
        print(url)
        url = normalize_url(url)
        name = None
        print(f"({url}, {name})")
        links.add((url, name))
    return links

def parse(url):
    html = get_html(url)
    title = parse_title(url, html)
    title = sanitize_str(title)
    dl_dir = get_dl_dir(url, title)
    save_page(html, title, dl_dir)
    links = set()
    links.update(parse_tag(html))
    links.update(parse_dtag(html))
    links.update(parse_mp3(html))
    links.update(parse_hidden(html))
    return links, dl_dir

def download(links, dl_dir):
    nerror = 0
    links = list(links)
    length = len(links)
    for i in range(length):
        url, name = links[i]
        try:
            print(f"\r[{i+1}/{length}] downloading: {url} .", flush=True)
            if url.endswith((".gif",".jpg",".jpeg",".png",".webp",".webm",".mp3",".mp4")):
                o = ["-o", name] if name else []
                subprocess.run(["aria2c", "-c", "-d", dl_dir, *o, url],
                               stdout=sys.stderr,
                               timeout=TIMEOUT,
                               check=True)
                print(f"\r[{i+1}/{length}] downloaded: {url} .", flush=True)
            elif not "boards.4chan.org" in url:
                if "nyaa.si/view" in url:
                    m = re.search(r'nyaa.si/view/([0-9]*)', url)
                    if m:
                        num = m.group(1)
                        url = f"https://nyaa.si/download/{num}.torrent"
                scheme  = os.getenv("SCHEME")
                port    = os.getenv("PORT")
                subprocess.run(["curl","-k", "-X","POST","-d", url, f"{scheme}://localhost:{port}"],
                               stdout=sys.stderr,
                               timeout=TIMEOUT,
                               check=True)
                with open(os.path.join(dl_dir, "repost.list"), "a") as f:
                    f.write("\n" + url)
                print(f"\r[{i+1}/{length}] repost:     {url} .", flush=True)
            time.sleep(0.3)
        except Exception as e:
            nerror += 1
            print(e, file=sys.stderr)
            msg = " ".join(str(e).split())
            with open(os.path.join(dl_dir, "error.list"), "a") as f:
                f.write("\n" + url)
                f.write("\n#"+ msg )
    return nerror

def main(args):
    err = 0
    try:
        check_command("curl")
        check_command("aria2c")
        for url in args:
            links, dl_dir = parse(url)
            err += download(links, dl_dir)
        if 0 < err:
            sys.exit(1)
    except Exception as e:
        print(e, file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main(sys.argv[1:])

