clean=${1:-0};
shouldFetch=${2:-1};
buildFramework=${3:-1};
source source-build.sh;
scriptDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

echo market-build.sh clean=$clean shouldFetch=$shouldFetch buildFramework=$buildFramework

if [ $buildFramework -eq 1 ]; then
 	$scriptDir/../Framework/framework-build.sh $clean $shouldFetch; if [ $? -ne 0 ]; then echo framework-build.sh failed - $?; exit 1; fi;
else
	#echo market-build says sourceBuild=\'$sourceBuild\';
	if [[ -z $sourceBuild ]]; then source source-build.sh; fi;
	toBashDir $REPO_DIR REPO_BASH
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
	fi;
	cd $previousDir;
}
xzBuildLib
echo `pwd`;
fetchBuild XZ 0

function protocBuild()
{
	publicDir=$2;
	if [ ! -f $1.pb.cc ]; then
		if [ $publicDir -eq 1 ]; then
			prevDir=`pwd`;
			workDir=$baseDir/$jdeRoot/Public/jde/markets/types/proto;
			cd $workDir;
		fi;
		cmd="protoc --cpp_out dllexport_decl=JDE_MARKETS_EXPORT:. $1.proto";
		$cmd;
		if [ $? -ne 0 ]; then
			echo `pwd`;
			echo $cmd;
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
	cd types/proto;
	if [ $clean -eq 1 ]; then
		rm *.pb.cc
		rm *.pb.h
	fi;
	protocBuild watch;
	protocBuild requests;
	protocBuild results;
	#sed -i 's/class Fundamentals_ValuesEntry_DoNotUse/class JDE_MARKETS_EXPORT Fundamentals_ValuesEntry_DoNotUse/' results.pb.h
	#sed -i 's/class StringMap_ValuesEntry_DoNotUse/class JDE_MARKETS_EXPORT StringMap_ValuesEntry_DoNotUse/' results.pb.h
	protocBuild ib;
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
		#else
			#echo Found \"$twsDir/TwsSocketClient64.vcxproj\";
		fi;
	 	prevDir=`pwd`;
		echo cd "$twsDir";
	 	cd "$twsDir";
		#echo buildWindows TwsSocketClient64;
	 	buildWindows TwsSocketClient64 TwsSocketClient64.dll;
	 	cd $prevDir;
		moveToDir .bin;
		moveToDir debug;
		#echo pwd=`pwd`;
		#echo mklink TwsSocketClient.lib "$twsDir/.bin/debug";
		mklink TwsSocketClient64.lib "$twsDir/.bin/debug";
		cd ..;
		moveToDir release;
		mklink TwsSocketClient64.lib "$twsDir/.bin/release";
		cd ../..;
	else
		echo Not Found:  \"c:\\TWS API\"
	fi;
}
fetchDefault MarketLibrary 0; 
marketLibraryProtoc;
build MarketLibrary 0 Jde.Markets.dll;