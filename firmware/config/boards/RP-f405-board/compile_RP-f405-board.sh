#!/bin/bash
source config/boards/common_script_read_meta_env.inc config/boards/RP-f405-board/meta-info.env
make -j$(nproc) BOARD_DIR=config/boards/RP-f405-board
