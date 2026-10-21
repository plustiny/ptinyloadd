# Makefile
CC := gcc
CFLAGS := -Warray-bounds=2 -Wnull-dereference -Wuse-after-free=3 -Wall -Wextra -g -O0 -Wno-deprecated-declarations
TARGET := ptinyloadd
SRC := main.c config.c job_list.c script_list.c http.c utf8.c
PKGS := libmicrohttpd jansson libcrypto zlib
PREFIX := /usr/local

CFLAGS  += $(shell pkg-config --cflags $(PKGS))
LDFLAGS += $(shell pkg-config --libs $(PKGS))
LDFLAGS += -lwrap

all: $(TARGET)

backup: BackupDir:=$(shell ls -tr1d .backup* 2>/dev/null | tail -1 )
backup: NewBackupDir:=$(shell date +'.backup_%Y-%m-%d_%H-%M-%S') 
backup:
	if [ "x$(BackupDir)" = "x" ];then rsync -av . $(NewBackupDir); else rsync --exclude=".*" -av --link-dest=../$(BackupDir) . $(NewBackupDir);fi

$(TARGET): $(SRC)
	$(CC) $(CFLAGS)  $(SRC) $(LDFLAGS) -o $(TARGET)

check:
	cppcheck $(SRC)

clean:
	rm -f $(TARGET)
	rm -f /tmp/ptinyloadd.*

install: all
	-groupadd share
	-useradd -s /bin/false -g share ptinyload
	mkdir -pm 0755 $(PREFIX)/etc/ptinyloadd/
	mkdir -pm 0755 $(PREFIX)/libexec/ptinyloadd/
	cp -f ptinyloadd $(PREFIX)/bin/
	-cp -f ptinyloadd.conf $(PREFIX)/etc/ptinyloadd/
	cp -fr ./libexec/ptinyloadd/* $(PREFIX)/libexec/ptinyloadd/
	cp -f ptinyloadd.service /etc/systemd/system/
	chown -R ptinyload:share $(PREFIX)/etc/ptinyloadd/
	chown -R ptinyload:share $(PREFIX)/libexec/ptinyloadd/

service:
	systemctl daemon-reload
	systemctl enable ptinyloadd
	-systemctl stop ptinyloadd
	systemctl start ptinyloadd

uninstall:
	-systemctl stop ptinyloadd
	-systemctl disable ptinyloadd
	rm -rf $(PREFIX)/etc/ptinyloadd/
	rm -rf $(PREFIX)/libexec/ptinyloadd/
	rm -f  $(PREFIX)/bin/ptinyloadd
	rm -f  /etc/systemd/system/ptinyloadd.service

gitupload:
	git add .
	git commit -m "$$(date -Is)" || true
	git push -u origin main

-include .makefile.mk
.PHONY: gitupload
.PHONY: all clean
