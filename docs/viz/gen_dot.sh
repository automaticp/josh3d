#!/usr/bin/bash

dir="${BASH_SOURCE%/*}"
cd "${dir}"

src_dir="../../"
build_dir="$src_dir/build/"
out_dir="./out-dot/"

cp "CMakeGraphVizOptions.cmake" "$build_dir"

cmake -S "$src_dir" -B "$build_dir" --graphviz="$out_dir/josh3d.dot"

