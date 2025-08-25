
# ./gradlew -Ptest_flags="-memory-model rtgc -g -Xbinary=stripDebugInfoFromNativeLibs=false" :kotlin-native:backend.native:tests:$1
# Xstatic-framework linker-options
# -g -Xg-generate-debug-trampoline=enable \
./kotlin-native/dist/bin/kotlinc-native -progressive -verbose \
    -Xruntime-logs=gc=info -Xbinary=gc=nstw \
    -Xdebug-info-version=2 \
    z_rtgc/tests/$1.kt -o z_rtgc/tests/out/$1.kexe
# ./z_rtgc/tests/out/$1.kexe