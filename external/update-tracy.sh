#!/usr/bin/env bash
cd "$(dirname "$0")"
libname=tracy
url=https://github.com/wolfpld/tracy
branch="v0.12.1"
if [ -d $libname ]; then rm -rf $libname; fi
git clone -b $branch --single-branch --depth 1 $url $libname \
    && cd $libname \
    && git rev-parse HEAD > ../$libname.rev \
    && rm -rf .git
