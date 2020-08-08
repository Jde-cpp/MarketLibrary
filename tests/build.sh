#!/bin/bash
type=${1:-asan}
clean=${2:-0}
all=${3:-1}
export CXX=clang++
if [ $all -eq 1 ]; then
	../../Framework/cmake/buildc.sh ../../Framework/source $type $clean || exit 1;
	../../Framework/cmake/buildc.sh ../../MarketLibrary/source $type $clean || exit 1;
fi

if [ ! -d .obj ]; then mkdir .obj; fi;

../../Framework/cmake/buildc.sh `pwd` $type $clean

exit $?
