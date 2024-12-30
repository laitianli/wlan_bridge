#!/bin/bash

PWD=$(pwd)
export LD_LIBRARY_PATH=${PWD}/../src_lib/libcli/
${PWD}/bin/wlan_br_server

