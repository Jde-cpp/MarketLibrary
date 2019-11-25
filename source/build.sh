#!/bin/bash
debug=${1:-1}
clean=${2:-0}

#echo all=$all
cd "${0%/*}"
echo "Build markets"
if [ $clean -eq 1 ]; then
	make -C. clean DEBUG=$debug
	if [ $debug -eq 1 ]; then
		ccache g++-8 -c -g -pthread -fPIC -std=c++17 -Wall -Wno-unknown-pragmas -DBOOST_SYSTEM_NO_DEPRECATED -DJDE_EXPORT_MARKETS -O0 pc.h -o.obj/debug/stdafx.h.gch -I/home/duffyj/code/libraries/spdlog/include -I/home/duffyj/code/libraries/tws-api/source/cppclient/client -I/home/duffyj/code/libraries/json/include -I$BOOST_ROOT -I../../Framework/source
# -I/home/duffyj/code/libraries/eigen -I/home/duffyj/code/libraries/json/include		
	else
		ccache g++-8 -c -g -pthread -fPIC -std=c++17 -Wall -DJDE_EXPORT_MARKETS -DBOOST_SYSTEM_NO_DEPRECATED -Wno-unknown-pragmas -march=native -DNDEBUG -O3 -I.obj/release pc.h -o.obj/release/stdafx.h.gch -I/home/duffyj/code/libraries/spdlog/include -I/home/duffyj/code/libraries/tws-api/source/cppclient/client -I/home/duffyj/code/libraries/json/include -I$BOOST_ROOT  -I../../Framework/source
#-I/home/duffyj/code/libraries/spdlog/include -I/home/duffyj/code/libraries/eigen -I/home/duffyj/code/libraries/json/include
	fi
	if [ $? -eq 1 ]; then
		exit 1
	fi
fi
make -C. -j7 DEBUG=$debug
cd -
exit $?