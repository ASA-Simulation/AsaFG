set -x
set -e

echo "Building simgear dependencies in debug mode"
conan create ./deps/simgear  --build=missing --settings=build_type=Debug

echo "Building mixr dependencies in release mode"
conan create ./deps/simgear  --build=missing --settings=build_type=Release
