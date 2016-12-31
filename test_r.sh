#!/bin/sh

sleep 1
./app -r -o app.out --localid 20 --remoteid 21 --port 10 -k key.pub -i key --decryptpvt
sleep 1
./app -r -o app.out --localid 20 --remoteid 21 --port 10 -k key.pub -i key --decryptpub
sleep 1
./app -r -o app.out --localid 20 --remoteid 21 --port 10
