#!/bin/bash

pwd_dir="$( cd "$( dirname "$0" )" && pwd )"
build_out_dir=$pwd_dir/../output_build

mkdir -p $build_out_dir

g++ $pwd_dir/../one_to_one/server.c -o $build_out_dir/server.app
g++ $pwd_dir/../one_to_one/client.c -o $build_out_dir/client.app
