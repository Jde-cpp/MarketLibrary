#!/bin/bash
clean=${1:-0};
shouldFetch=${2:-1};
buildFramework=${3:-1};
#source source-build.sh;
scriptDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
stage=$REPO_DIR/jde/Public/stage
echo market-build.sh clean=$clean shouldFetch=$shouldFetch buildFramework=$buildFramework
if [[ -z $sourceBuild ]]; then source $scriptDir/../Framework/source-build.sh; fi;
if [ $buildFramework -eq 1 ]; then
 	$scriptDir/../Framework/framework-build.sh $clean $shouldFetch; if [ $? -ne 0 ]; then echo framework-build.sh failed - $?; exit 1; fi;
else
	findExecutable protoc.exe $REPO_BASH/protobuf/cmake/build/sln/Release;
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
		#else
			#echo already exists:  $dir/bin_x86-64/liblzma.lib;
		fi;
	else
		echo Not found:  $dir.
		exit 1;
	fi;
	cd $previousDir;
}
xzBuildLib
cd $REPO_DIR/jde;
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
		fi;
	fi;
}

function marketLibraryProtoc
{
	findExecutable protoc.exe $stage/release;
	echo marketLibraryProtoc;
	cd types/proto;
	if [ $clean -eq 1 ]; then
		rm *.pb.cc
		rm *.pb.h
	fi;
	protocBuild watch 1;
	protocBuild requests 1;
	protocBuild results 1;
	pushd `pwd` > /dev/null;
	cd $baseDir/$jdeRoot/Public/jde/markets/types/proto;
	sed -i 's/class Fundamentals_ValuesEntry_DoNotUse/class JDE_MARKETS_EXPORT Fundamentals_ValuesEntry_DoNotUse/' results.pb.h;
	sed -i 's/class StringMap_ValuesEntry_DoNotUse/class JDE_MARKETS_EXPORT StringMap_ValuesEntry_DoNotUse/' results.pb.h;
	popd;
	protocBuild ib 1;
	protocBuild OptionOI;
	protocBuild bar;
	protocBuild edgar 1;
	cd ../..;
	twsDir=/c/TWS\ API/source/CppClient/client;
	if [ -d  "$twsDir" ]; then
		if [ ! -f "$twsDir/TwsSocketClient64.vcxproj" ]; then
			cd ..;
			sourceDir=`pwd`;
			cd "$twsDir"
			mklink TwsSocketClient64.vcxproj $sourceDir
			cd $sourceDir;
		fi;
		if [ ! -f  $stage/release/TwsSocketClient64.dll ]; then
			prevDir=`pwd`;
			cd "$twsDir";
			buildWindows TwsSocketClient64 TwsSocketClient64.dll;
			cd $stage/debug;
			mklink TwsSocketClient64.lib "$twsDir/.bin/debug"; mklink TwsSocketClient64.dll "$twsDir/.bin/debug";
			cd $stage/release;
			mklink TwsSocketClient64.lib "$twsDir/.bin/release"; mklink TwsSocketClient64.dll "$twsDir/.bin/release";
			cd $prevDir;
		fi;
	else
		echo Not Found:  \"c:\\TWS API\"
	fi;
}
fetchDefault MarketLibrary 0;
echo build fetchDefault MarketLibrary 0;
marketLibraryProtoc;
echo build MarketLibrary;
build MarketLibrary 0 Jde.Markets.dll $clean;