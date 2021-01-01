clean=${1:-0};
windows=${2:-0};
fetch=${3:-1};
echo 'hi'
# ./framework-build.sh $clean $windows $fetch

# function marketLibraryProtoc
# {
# 	cleanProtoc=$clean
# 	cd types/proto;
# 	if [ ! -f ib.pb.cc ]; then
# 		cleanProtoc=1;
# 	fi;
# 	if [ $cleanProtoc -eq 1 ]; then
# 		echo marketLibraryProtoc running;
# 	   protoc --cpp_out dllexport_decl=JDE_MARKETS_EXPORT:. requests.proto;
# 	   protoc --cpp_out dllexport_decl=JDE_MARKETS_EXPORT:. results.proto;
# 	   protoc --cpp_out dllexport_decl=JDE_MARKETS_EXPORT:. ib.proto;
# 	fi;
# 	cd ../..
# }
# build jde/MarketLibrary 0 'marketLibraryProtoc'