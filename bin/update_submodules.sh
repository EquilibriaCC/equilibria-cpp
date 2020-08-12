#!/usr/bin/env bash

# initialize submodules recursively
git submodule update --init --recursive

# update monero-core
cd ./external/equilibria
git checkout tags/v6.0.3
git pull --ff-only origin tags/v6.0.3
cd ../../