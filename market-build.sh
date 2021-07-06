#!/bin/bash
clean=${1:-0};
shouldFetch=${2:-1};
buildFramework=${3:-1};
scriptDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
stage=$REPO_DIR/jde/Public/stage
echo market-build.sh clean=$clean shouldFetch=$shouldFetch buildFramework=$buildFramework
if [[ -z $sourceBuild ]]; then source $scriptDir/../Framework/source-build.sh; fi;
if [ $buildFramework -eq 1 ]; then
 	$scriptDir/../Framework/framework-build.sh $clean $shouldFetch $buildBoost; if [ $? -ne 0 ]; then echo framework-build.sh failed - $?; exit 1; fi;
else
	findProtoc
fi;
function xzBuildLib
{
	previousDir=`pwd`;
	dir=$REPO_BASH/xz-5.2.5-windows;
	if [ -d $dir ]; then
		cd $dir/bin_x86-64;
		if [ ! -f liblzma.lib ]; then
			dumpbin //EXPORTS liblzma.dll > liblzma.exports;
			printf "LIBRARY liblzma\nEXPORTS\n" > liblzma.def;
			grep -o '\blzma_\w*' liblzma.exports >> liblzma.def;
			lib //def:liblzma.def //machine:x64 //out:liblzma.lib > /dev/null;
			echo created $dir/bin_x86-64/liblzma.lib;
			rm liblzma.exports;
			rm liblzma.def;
			rm liblzma.exp;
		fi;
	else
		echo Not found:  $dir.
		exit 1;
	fi;
	cd $previousDir;
}
if windows; then xzBuildLib; fi;
cd $scriptDir/..;
fetchBuild XZ 0
function protocBuild()
{
	publicDir=${2:-0};
	if [ ! -f $1.pb.cc ]; then
		if [ $publicDir -eq 1 ]; then
			prevDir=`pwd`;
			workDir=$baseDir/$jdeRoot/Public/jde/markets/types/proto;
			cd $workDir;
		fi;
		cmd2="protoc --cpp_out dllexport_decl=JDE_MARKETS_EXPORT:. $1.proto";
		$cmd2;
		if [ $? -ne 0 ]; then
			echo `pwd`;
			echo $cmd2;
			exit 1;
		fi;
		if [ $publicDir -eq 1 ]; then
			cd $prevDir;
			mv $workDir/$1.pb.cc .;
			if ! windows; then
				ln -s $workDir/$1.pb.h .;
			fi;
		fi;
	fi;
}

function marketLibraryProtoc
{
	findProtoc
	echo marketLibraryProtoc;
	cd types/proto;
	if [ $clean -eq 1 ]; then
		rm *.pb.cc > /dev/null;
		rm *.pb.h > /dev/null;
	fi;
	protocBuild watch 1;
	protocBuild requests 1;
	protocBuild results 1;
	pushd `pwd` > /dev/null;
	cd $baseDir/$jdeRoot/Public/jde/markets/types/proto;
	if windows; then
		sed -i 's/class Fundamentals_ValuesEntry_DoNotUse/class JDE_MARKETS_EXPORT Fundamentals_ValuesEntry_DoNotUse/' results.pb.h;
		sed -i 's/class StringMap_ValuesEntry_DoNotUse/class JDE_MARKETS_EXPORT StringMap_ValuesEntry_DoNotUse/' results.pb.h;
	fi;
	popd;
	protocBuild ib 1;
	protocBuild OptionOI;
	protocBuild bar;
	protocBuild edgar 1;
	cd ../..;
	if windows; then
		twsDir=/c/TWS\ API/source/CppClient/client;
		if [ -d  "$twsDir" ]; then
			echo Found:  \"c:\\TWS API\"
			if [ ! -f "$twsDir/TwsSocketClient64.vcxproj" ]; then
				cd ..;
				sourceDir=`pwd`;
				cd "$twsDir"
				mklink TwsSocketClient64.vcxproj $sourceDir
				cd $sourceDir;
			fi;
			if [ ! -f  $stage/release/TwsSocketClient.dll ]; then
				prevDir=`pwd`;
				cd "$twsDir";
				buildWindows TwsSocketClient64 TwsSocketClient.dll;
				cd $stage/debug;
				mklink TwsSocketClient.lib "$twsDir/.bin/debug"; mklink TwsSocketClient.dll "$twsDir/.bin/debug";
				cd $stage/release;
				mklink TwsSocketClient.lib "$twsDir/.bin/release"; mklink TwsSocketClient.dll "$twsDir/.bin/release";
				cd $prevDir;
			fi;
		else
			echo Not Found:  \"c:\\TWS API\"
		fi;
	fi;
}
fetchDefault MarketLibrary 0;
echo build fetchDefault MarketLibrary 0;
marketLibraryProtoc;
echo build MarketLibrary;
build MarketLibrary 0 Jde.Markets.dll $clean;