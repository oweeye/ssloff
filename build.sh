conan install . --profile=clang-18 --build=missing
# conan-release
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON --preset conan-debug
make -C build/Debug/
[ -L ssloff ] || ln -s ./build/Debug/src/ssloff ssloff
[ -L compile_commands.json ] || ln -s ./build/Debug/compile_commands.json compile_commands.json
./build/Debug/tests/tests $@
make -C ./build/Debug/ && ./build/Debug/tests/tests
