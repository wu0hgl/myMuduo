#!/bin/sh

set -x                              # 追踪所有执行命令

SOURCE_DIR=`pwd`                    # 反斜号, 执行命令
BUILD_DIR=${BUILD_DIR:-../build}    # BUILD_DIR判断, 为空取build值

mkdir -p $BUILD_DIR \
  && cd $BUILD_DIR \
  && cmake $SOURCE_DIR \
  && make $*
