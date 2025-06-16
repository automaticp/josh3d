#!/usr/bin/env bash
cd "$(dirname "$0")"
if [ -d imgui ]; then rm -rf imgui; fi
git clone -b docking --single-branch --depth 1 https://github.com/ocornut/imgui \
    && cd imgui \
    && git rev-parse HEAD > ../imgui.rev \
    && rm -rf .git
