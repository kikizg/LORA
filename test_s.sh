#!/bin/sh

./app -t -f app.in --localid 21 --remoteid 20 --port 10 -k key.pub -i key --encryptpub
sleep 1
./app -t -f app.in --localid 21 --remoteid 20 --port 10 -k key.pub -i key --encryptpvt
sleep 1
./app -t -f app.in --localid 21 --remoteid 20 --port 10
