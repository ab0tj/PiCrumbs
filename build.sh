#! /bin/bash

g++ main.cpp -lpthread -lhamlib -lhamlib++ -lwiringPi -lcurl
mv a.out picrumbs
