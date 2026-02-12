#!/usr/bin/env bash
cd "$(dirname "$0")"
libname=imguizmo
if [ -d $libname ]; then rm -rf $libname; fi
git clone -b master --single-branch --depth 1 https://github.com/CedricGuillemet/ImGuizmo $libname \
    && cd $libname \
    && git rev-parse HEAD > ../$libname.rev \
    && rm -rf .git
