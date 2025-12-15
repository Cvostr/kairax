#!/bin/bash

cd nvme
make
cd ..

cd ahci
make
cd ..

cd fat
make
cd ..

cd hid
make
cd ..