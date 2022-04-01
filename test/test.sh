#!/usr/bin/env bash

cp ../cstorage ./
cp ../cstorage-client ./
mkdir -p servdir
python3 test.py && echo "tests passed"
rm servdir -rf
rm cstorage cstorage-client

