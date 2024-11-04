
./gradlew -Ptest_flags="-memory-model rtgc -g -Xbinary=stripDebugInfoFromNativeLibs=false" :kotlin-native:backend.native:tests:$1
