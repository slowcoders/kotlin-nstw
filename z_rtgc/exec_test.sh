
# ./gradlew -Ptest_flags="-memory-model rtgc -g -Xbinary=stripDebugInfoFromNativeLibs=false" :kotlin-native:backend.native:tests:$1
./kotlin-native/dist/bin/kotlinc-native -Xruntime-logs=gc=info -target macos_arm64 -Xbinary=gc=nstw \
    z_rtgc/tests/$1.kt -o z_rtgc/tests/out/$1.kexe
./z_rtgc/tests/out/$1.kexe