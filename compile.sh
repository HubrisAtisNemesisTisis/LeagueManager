#!/bin/bash

set -e

if [ -f LeagueofLegendsManager.exe ]; then
  rm LeagueofLegendsManager.exe
fi

if [ -d build ]; then
  rm -rf build
fi

mkdir -p build

cd build

cmake -G Ninja ..

ninja

cd ..