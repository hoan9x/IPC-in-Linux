#!/bin/bash

pwd_dir="$( cd "$( dirname "$0" )" && pwd )"
build_out_dir=$pwd_dir/../output_build

mkdir -p $build_out_dir

gcc $pwd_dir/../one_to_one/server.c -o $build_out_dir/server.app
gcc $pwd_dir/../one_to_one/client.c -o $build_out_dir/client.app

gcc $pwd_dir/../one_to_many/server.c -o $build_out_dir/multiplexing_server.app
gcc $pwd_dir/../one_to_many/server2.c -o $build_out_dir/multiplexing_server2.app
gcc $pwd_dir/../one_to_many/server3.c -o $build_out_dir/multiplexing_server3.app
gcc $pwd_dir/../one_to_many/client.c -o $build_out_dir/many_client.app
