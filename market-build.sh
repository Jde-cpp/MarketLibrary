clean=${1:-0};
fetch=${2:-1};
buildFramework=${3:-1};

scriptDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

echo market-build.sh clean=$clean fetch=$fetch buildFramework=$buildFramework

if [ $buildFramework -eq 1 ]; then
 	$scriptDir/../Framework/framework-build.sh $clean $fetch
else
	#echo source $scriptDir/../Framework/source-build.sh;
	source $scriptDir/../Framework/source-build.sh;
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
		else
			echo already exists:  $dir/bin_x86-64/liblzma.lib;
		fi;
	else
		echo Not found:  $dir.
	fi;
	cd $previousDir;
}
xzBuildLib
fetchBuild XZ 0

function protocBuild()
{
	if [ ! -f $1.pb.cc ]; then
		protoc --cpp_out dllexport_decl=JDE_MARKETS_EXPORT:. $1.proto;
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
	protocBuild ib;
	protocBuild OptionOI;
	protocBuild bar;
	protocBuild edgar;
	cd ../..;
	twsDir=/c/TWS\ API/source/CppClient/client;
	if [ -d  "$twsDir" ]; then
		if [ ! -f "$twsDir/TwsSocketClient64.vcxproj" ]; then
			cd ..;
			sourceDir=`pwd`;
			cd "$twsDir"
			mklink TwsSocketClient64.vcxproj $sourceDir
			cd $sourceDir;
		else
			echo Found \"$twsDir/TwsSocketClient64.vcxproj\";
		fi;
	 	prevDir=`pwd`;
	 	cd "$twsDir";
	 	buildWindows TwsSocketClient64;
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
#fetchDefault MarketLibrary 0; 
marketLibraryProtoc;
build MarketLibrary;