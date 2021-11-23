#!/bin/bash
clean=${1:-0};
shouldFetch=${2:-1};
buildFramework=${3:-1};
scriptDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
echo market-build.sh clean=$clean shouldFetch=$shouldFetch buildFramework=$buildFramework
#echo `pwd`
source $scriptDir/../Framework/common.sh;
source $scriptDir/../Framework/source-build.sh;
# if [[ -z $REPO_BASH ]]; then toBashDir $REPO_DIR REPO_BASH; fi;
# if [[ -z $JDE_DIR ]]; then JDE_BASH=$REPO_BASH/jde; else toBashDir $JDE_DIR JDE_BASH; fi;
#echo JDE_BASH=$JDE_BASH;
#if [[ -z $sourceBuild ]]; then source $JDE_BASH/Framework/source-build.sh; fi;
#cd $JDE_BASH;
#echo pwd=`pwd`
#if [[ -z $sourceBuild ]]; then source $JDE_BASH/Framework/source-build.sh; fi;
stage=$JDE_BASH/Public/stage
if [ $buildFramework -eq 1 ]; then
 	$JDE_BASH/Framework/framework-build.sh $clean $shouldFetch $buildBoost; if [ $? -ne 0 ]; then echo framework-build.sh failed - $?; exit 1; fi;
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
			binDir=`pwd`;
			cd $JDE_BASH/Public/stage/release;
			mklink liblzma.dll $binDir;
			cd ../debug;
			mklink liblzma.dll $binDir;
			cd $binDir;
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
cd $JDE_BASH;
fetchBuild XZ 0
function protocBuild()
{
	if [ -f $1.pb.cc ]; then return 1; fi;
	publicDir=${2:-0};
	if [ $publicDir -eq 1 ]; then
		prevDir=`pwd`;
		workDir=$JDE_BASH/Public/jde/markets/types/proto;
		cd $workDir;
	fi;
	cmd2="protoc --cpp_out dllexport_decl=JDE_MARKETS_EXPORT:. $1.proto";
	#ΓM
	#echo `pwd`
	#echo $cmd2;
	$cmd2;
	if [ $? -ne 0 ]; then echo `pwd`; echo $cmd2; exit 1; fi;
	sed -i -e 's/JDE_MARKETS_EXPORT/ΓM/g' $1.pb.h;
	sed -i '1s/^/\xef\xbb\xbf/' $1.pb.h;
	if [ $publicDir -eq 1 ]; then
		cd $prevDir;
		mv $workDir/$1.pb.cc .;
		if ! windows; then
			ln -s $workDir/$1.pb.h .;
		fi;
	fi;
	return 0;
}

function marketLibraryProtoc
{
	pushd `pwd` > /dev/null;
	findProtoc
	echo marketLibraryProtoc;
	cd types/proto;
	if [ $clean -eq 1 ]; then
		rm *.pb.cc > /dev/null;
		rm *.pb.h > /dev/null;
	fi;
	#sed -i 's/~/~/' watch.pb.cc;
	if protocBuild watch 1 && windows; then
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT FileDefaultTypeInternal _File_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY FileDefaultTypeInternal _File_default_instance_;/' watch.pb.cc;
	fi;
	if protocBuild requests 1 && windows; then
		echo fix requests.pb.cc `pwd`
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT StringRequestDefaultTypeInternal _StringRequest_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY StringRequestDefaultTypeInternal _StringRequest_default_instance_;/' requests.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT RequestAccountUpdatesDefaultTypeInternal _RequestAccountUpdates_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY RequestAccountUpdatesDefaultTypeInternal _RequestAccountUpdates_default_instance_;/' requests.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT RequestAccountUpdatesMultiDefaultTypeInternal _RequestAccountUpdatesMulti_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY RequestAccountUpdatesMultiDefaultTypeInternal _RequestAccountUpdatesMulti_default_instance_;/' requests.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT RequestPositionsDefaultTypeInternal _RequestPositions_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY RequestPositionsDefaultTypeInternal _RequestPositions_default_instance_;/' requests.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT FlexExecutionsDefaultTypeInternal _FlexExecutions_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY FlexExecutionsDefaultTypeInternal _FlexExecutions_default_instance_;/' requests.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT PlaceOrderDefaultTypeInternal _PlaceOrder_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY PlaceOrderDefaultTypeInternal _PlaceOrder_default_instance_;/' requests.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT RequestExecutionsDefaultTypeInternal _RequestExecutions_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY RequestExecutionsDefaultTypeInternal _RequestExecutions_default_instance_;/' requests.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT RedditDefaultTypeInternal _Reddit_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY RedditDefaultTypeInternal _Reddit_default_instance_;/' requests.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT CustomDefaultTypeInternal _Custom_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY CustomDefaultTypeInternal _Custom_default_instance_;/' requests.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT NewsArticleRequestDefaultTypeInternal _NewsArticleRequest_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY NewsArticleRequestDefaultTypeInternal _NewsArticleRequest_default_instance_;/' requests.pb.cc;
	fi;
	if protocBuild results 1 && windows; then
		echo fix results.pb.cc `pwd`
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT StringResultDefaultTypeInternal _StringResult_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY StringResultDefaultTypeInternal _StringResult_default_instance_;/' results.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT GoogleNewsDefaultTypeInternal _GoogleNews_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY GoogleNewsDefaultTypeInternal _GoogleNews_default_instance_;/' results.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT NewsArticleDefaultTypeInternal _NewsArticle_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY NewsArticleDefaultTypeInternal _NewsArticle_default_instance_;/' results.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT TickNewsDefaultTypeInternal _TickNews_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY TickNewsDefaultTypeInternal _TickNews_default_instance_;/' results.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT HistoricalNewsDefaultTypeInternal _HistoricalNews_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY HistoricalNewsDefaultTypeInternal _HistoricalNews_default_instance_;/' results.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT TagValueDefaultTypeInternal _TagValue_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY TagValueDefaultTypeInternal _TagValue_default_instance_;/' results.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT ContractDetailDefaultTypeInternal _ContractDetail_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY ContractDetailDefaultTypeInternal _ContractDetail_default_instance_;/' results.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT PositionDefaultTypeInternal _Position_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY PositionDefaultTypeInternal _Position_default_instance_;/' results.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT StringMap_ValuesEntry_DoNotUseDefaultTypeInternal _StringMap_ValuesEntry_DoNotUse_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY StringMap_ValuesEntry_DoNotUseDefaultTypeInternal _StringMap_ValuesEntry_DoNotUse_default_instance_;/' results.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT StringMapDefaultTypeInternal _StringMap_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY StringMapDefaultTypeInternal _StringMap_default_instance_;/' results.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT Fundamentals_ValuesEntry_DoNotUseDefaultTypeInternal _Fundamentals_ValuesEntry_DoNotUse_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY Fundamentals_ValuesEntry_DoNotUseDefaultTypeInternal _Fundamentals_ValuesEntry_DoNotUse_default_instance_;/' results.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT FundamentalsDefaultTypeInternal _Fundamentals_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY FundamentalsDefaultTypeInternal _Fundamentals_default_instance_;/' results.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT AccountUpdateDefaultTypeInternal _AccountUpdate_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY AccountUpdateDefaultTypeInternal _AccountUpdate_default_instance_;/' results.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT AccountUpdateMultiDefaultTypeInternal _AccountUpdateMulti_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY AccountUpdateMultiDefaultTypeInternal _AccountUpdateMulti_default_instance_;/' results.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT PositionMultiDefaultTypeInternal _PositionMulti_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY PositionMultiDefaultTypeInternal _PositionMulti_default_instance_;/' results.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT CommissionReportDefaultTypeInternal _CommissionReport_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY CommissionReportDefaultTypeInternal _CommissionReport_default_instance_;/' results.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT PortfolioUpdateDefaultTypeInternal _PortfolioUpdate_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY PortfolioUpdateDefaultTypeInternal _PortfolioUpdate_default_instance_;/' results.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT FlexOrderDefaultTypeInternal _FlexOrder_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY FlexOrderDefaultTypeInternal _FlexOrder_default_instance_;/' results.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT TradeDefaultTypeInternal _Trade_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY TradeDefaultTypeInternal _Trade_default_instance_;/' results.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT ExecutionDefaultTypeInternal _Execution_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY ExecutionDefaultTypeInternal _Execution_default_instance_;/' results.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT ErrorDefaultTypeInternal _Error_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY ErrorDefaultTypeInternal _Error_default_instance_;/' results.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT OrderStatusDefaultTypeInternal _OrderStatus_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY OrderStatusDefaultTypeInternal _OrderStatus_default_instance_;/' results.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT TweetDefaultTypeInternal _Tweet_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY TweetDefaultTypeInternal _Tweet_default_instance_;/' results.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT RedditDefaultTypeInternal _Reddit_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY RedditDefaultTypeInternal _Reddit_default_instance_;/' results.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT TweetAuthorDefaultTypeInternal _TweetAuthor_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY TweetAuthorDefaultTypeInternal _TweetAuthor_default_instance_;/' results.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT CustomDefaultTypeInternal _Custom_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY CustomDefaultTypeInternal _Custom_default_instance_;/' results.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT TickStringDefaultTypeInternal _TickString_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY TickStringDefaultTypeInternal _TickString_default_instance_;/' results.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT OrderStateDefaultTypeInternal _OrderState_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY OrderStateDefaultTypeInternal _OrderState_default_instance_;/' results.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT ExchangeContractsDefaultTypeInternal _ExchangeContracts_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY ExchangeContractsDefaultTypeInternal _ExchangeContracts_default_instance_;/' results.pb.cc;
		pushd `pwd` > /dev/null;
		cd $JDE_BASH/Public/jde/markets/types/proto;
		sed -i 's/class Fundamentals_ValuesEntry_DoNotUse/class ΓM Fundamentals_ValuesEntry_DoNotUse/' results.pb.h;
		sed -i 's/class StringMap_ValuesEntry_DoNotUse/class ΓM StringMap_ValuesEntry_DoNotUse/' results.pb.h;
		popd;
	fi;
	if protocBuild ib 1 && windows; then
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT ComboLegDefaultTypeInternal _ComboLeg_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY ComboLegDefaultTypeInternal _ComboLeg_default_instance_;/' ib.pb.cc;	
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT ContractDefaultTypeInternal _Contract_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY ContractDefaultTypeInternal _Contract_default_instance_;/' ib.pb.cc;	
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT OrderDefaultTypeInternal _Order_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY OrderDefaultTypeInternal _Order_default_instance_;/' ib.pb.cc;
	fi;
	protocBuild OptionOI;
	protocBuild bar;
	#protocBuild edgar 1;
	if protocBuild edgar 1 && windows; then
		echo fix edgar.pb.cc `pwd`
		#pushd `pwd` > /dev/null;
		#cd $JDE_BASH/Public/jde/markets/types/proto;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT InfoTableDefaultTypeInternal _InfoTable_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY InfoTableDefaultTypeInternal _InfoTable_default_instance_;/' edgar.pb.cc;
		sed -i 's/PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT CompanyDefaultTypeInternal _Company_default_instance_;/PROTOBUF_ATTRIBUTE_NO_DESTROY CompanyDefaultTypeInternal _Company_default_instance_;/' edgar.pb.cc;
		#popd;
	fi;
	#cd ../..;
	if [ ! -f  $stage/release/libbid.lib ]; then
		if [ ! -d $REPO_BASH/IntelRDFPMathLib20U2 ]; then echo install intel floating point library; exit 1; fi;
		cd $REPO_BASH/IntelRDFPMathLib20U2/LIBRARY;
		nmake -fmakefile.mak CC=cl CALL_BY_REF=0 GLOBAL_RND=0 GLOBAL_FLAGS=0 UNCHANGED_BINARY_FLAGS=0
		cd $JDE_BASH/Public/stage/debug/
		mklink libbid.lib $REPO_BASH/IntelRDFPMathLib20U2/LIBRARY
		cd ../release/
		mklink libbid.lib $REPO_BASH/IntelRDFPMathLib20U2/LIBRARY		
	fi;
	if windows; then
		twsDir=/c/TWS\ API/source/CppClient/client;
		if [ -d  "$twsDir" ]; then
			echo Found:  \"c:\\TWS API\"
			if [ ! -f "$twsDir/TwsSocketClient64.vcxproj" ]; then
				cd ..;
				sourceDir=$JDE_BASH/MarketLibrary;
				echo sourceDir=$sourceDir
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
	popd;
}
fetchDefault MarketLibrary 0;
echo marketLibraryProtoc;
marketLibraryProtoc;
echo build MarketLibrary;
build MarketLibrary 0 Jde.Markets.dll $clean;