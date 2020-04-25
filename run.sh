#!/usr/bin/env bash

docker run \
  -it \
  --mount type=bind,source="$(pwd)"/trunk/,target=/fteqw/ \
  --mount type=bind,source="$(pwd)"/output/,target=/output/ \
  fteqw
