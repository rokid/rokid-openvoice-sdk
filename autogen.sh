#!/bin/bash

if [ -z "$1" ]; then
	deps_dir=/usr/local
else
	deps_dir=$1
fi

gyp --depth=. -f make --generator-output=build -Ddeps_dir=${deps_dir} ${root_gyp}
echo "deps_dir := ${deps_dir}" > Makefile
cat Makefile.in >> Makefile
