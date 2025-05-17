#!/bin/env bash

if [ ! -d "build" ]; then
  mkdir build
fi

cd "build"

if [ ! -f "cma" ]; then
  echo "Building..." >&2
  cmake .. &>/dev/null
  make cma &>/dev/null
fi

./cma $1 &>/dev/null
echo $?
