#!/bin/sh
./usb-spi `motelist | grep DLRC | awk '{print $1}'` $1 $2

