#!/usr/bin/env bash

mkdir /tmp/fteqw_tmp/
cd /tmp/fteqw_tmp/ \
 && cmake /fteqw/ \
 && make \
 && cp -R * /output/
