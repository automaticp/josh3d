#!/usr/bin/bash

dir="${BASH_SOURCE%/*}"
cd "${dir}"

in_dir="./out-dot/"
out_dir="./out-pics/"

rm -v "$out_dir"/*.png

generate_subtarget() {
    echo "Generating $1 for $2"
    dot -K"$1" -Tpng -o"$out_dir/$2.$1.png" "$in_dir/josh3d.dot.$2"
}

generate_project() {
    echo "Generating $1 for project $2"
    dot -K"$1" -Tpng -o"$out_dir/project.$2.$1.png" "$in_dir/$2.dot"
}

for tgt_file in "$in_dir"/*.dot.*; do
    tgt=${tgt_file##*.}
    for fmt in dot circo; do
        generate_subtarget ${fmt} ${tgt}
    done
done

for fmt in dot circo; do
    generate_project ${fmt} "josh3d"
done
