#!/bin/bash
g++ -std=c++11 -I/usr/local/include/RF24 transfer.cpp -lrf24 -o transfer
g++ -std=c++11 -I/usr/local/include/RF24 transfer2.cpp -lrf24 -o transfer2
