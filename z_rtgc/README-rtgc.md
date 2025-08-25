## git version 
build-1.7.22-release-288
- git-lfs 설치
```sh
brew install git-lfs
git lfs install
git lfs install --system
```

## ref docs
- https://kotlinlang.org/docs/native-memory-manager.html#what-s-next 
- https://kotlinlang.org/docs/native-migration-guide.html 


## set gradle.properties
```sh
# for RTGC local properties
## to build kotlin-native
kotlin.native.enabled=true
## to skip to build JDK-1.6, JDK-1.7
kotlin.build.isObsoleteJdkOverrideEnabled=true
org.gradle.java.installations.auto-detect=false
```

## build
```sh
# kotlin-native core build. 20분.
./gradlew :kotlin-native:dist

# To get platform libraries, add distPlatformLibs task, e.g.
./gradlew :kotlin-native:dist :kotlin-native:distPlatformLibs

# To run the full build: 1시간 
./gradlew :kotlin-native:bundle
```

## run test 
```sh

./gradlew -Ptest_flags="-g -Xbinary=gc=nstw  -Xbinary=stripDebugInfoFromNativeLibs=false" :native:native.tests:stdlibTest

./gradlew -Ptest_flags="-g -Xbinary=gc=nstw  -Xbinary=stripDebugInfoFromNativeLibs=false" :native:native.tests:stress:test

./gradlew :nativeCompilerTest --continue

./gradlew :nativeCompilerTest --continue \
    -Pkotlin.internal.native.test.target=macos_arm64 \
    -Pkotlin.internal.native.test.gcType=NSTW STWMS or CMS, PCMS
    -Pkotlin.internal.native.test.compileOnly=false
    -Pkotlin.internal.native.test.optimizationMode=DEBUF OPT
    gcScheduler=UNSPECIFIED (default), ADAPTIVE, AGGRESSIVE, MANUAL
    alloc=UNSPECIFIED (default), STD, CUSTOM
    xctest=false (default), true.
    -Pkotlin.native.tests.tags='frontend-classic'

## CodeGen box test.
./gradlew :native:native.tests:codegen-box:test --tests \
 "org.jetbrains.kotlin.konan.test.blackbox.NativeCodegenBoxTestGenerated\$Box\$*"

./gradlew :native:native.tests:codegen-box:test --tests \
 "org.jetbrains.kotlin.konan.test.blackbox.NativeCodegenBoxTestGenerated\$Box\$Annotations"

## Runime Tests. GC 포함. C++ 
## output: ./kotlin-native/runtime/build/bin/test/macos_arm64/*.kexe
./gradlew :kotlin-native:runtime:hostRuntimeTests \
    -Pgtest_filter=STMS/STWMarkGCTest/0.MultipleMutatorsWeaks
    -Pgtest_filter=ThreadStateDeathTest.ReentrantStateSwitch_CalledFromNativeGuard
    -Pgtest_filter=MarkAndSweepUtilsSweepTest.SweepEmpty
    -Pgtest_filter=PMCS/TracingGCTest/300ParallelWithGCThreads.RootSet




## --------------------  old ------------------------

### kotlin-native language feature test
./gradlew --continue  -Pkotlin.internal.native.test.target=macos_arm64 -Ptest_flags="-g -Xbinary=gc=nstw -Xbinary=stripDebugInfoFromNativeLibs=false" :native:native.tests:test

./gradlew --continue -Ptest_flags="kotlin.internal.native.test.target=macos_arm64 -g -Xbinary=gc=nstw -Xbinary=stripDebugInfoFromNativeLibs=false" :native:native.tests:test

### kotlin-native language feature test for specific target
./gradlew -Ptest_target=wasm32 :kotlin-native:backend.native:tests:run

### specific test with debuging mode (주의. 일부 testcase는 debugging 모드 빌드 시 test 실패함.)
./gradlew -Ptest_flags="-g -Xbinary=gc=nstw -Xbinary=stripDebugInfoFromNativeLibs=false" :kotlin-native:backend.native:tests:cycle_detector

### blackbox test ???
./gradlew :native:native.tests:codegenBoxTest 
```

## Test reports.
- 아래 폴더에 test 결과 생성.
```sh
ls  kotlin-native/backend.native/tests/build/reports/
ex) kotlin-native/backend.native/tests/build/reports/tests/debugger_test/classes/org.jetbrains.kotlin.native.test.debugger.DwarfTests.html
```

## run benchmarks
```sh
## 참고) benchmarks 는 cache 된 platformLibs 사용.
# --> cache directory /Volumes/WorkSpace/kotlin-nstw/kotlin-native/dist/klib/cache/linux_x64-gSTATIC

### prepare platformLibs
# 주의) runtime 변경 사항이 있을 때 마다 rebuild 필요. (# use target of your laptop here instead ex) linux_arm64PlatformLibs)
./gradlew linux_x64PlatformLibs  


### benchmark analyzer 빌드.
cd kotlin-native/tools/benchmarksAnalyzer
../../../gradlew build
```

### run benchmarks. 80분
sh z_rtgc/run_benchmarks.sh

### single banchmark 실행
../../gradlew -PcompilerArgs="-g -Xbinary=gc=nstw" :cinterop:konanRun
../../gradlew -PcompilerArgs="-g -Xbinary=gc=nstw" :helloworld:runKonanC8

### debug benchmark compile option.
./gradlew linux_x64PlatformLibs && pushd kotlin-native/performance && ../../gradlew :ring:konanRun --filter=ForLoops.arrayIndicesLoop -PcompilerArgs="-g -Xbinary=gc=nstw -Xbinary=stripDebugInfoFromNativeLibs=false" && popd
./gradlew linux_x64PlatformLibs && pushd kotlin-native/performance && ../../gradlew :ring:konanRun --filter=GraphSolver.solve -PcrossTarget="linuxX64" -PcompilerArgs="-g -Xbinary=gc=nstw -Xbinary=stripDebugInfoFromNativeLibs=false" && popd
./gradlew linux_x64PlatformLibs && pushd kotlin-native/performance && ../../gradlew :ring:konanRun --filter=IntArray.copy -PcompilerArgs="-g -Xbinary=gc=nstw -Xbinary=stripDebugInfoFromNativeLibs=false" && popd
./gradlew linux_x64PlatformLibs && pushd kotlin-native/performance && ../../gradlew :ObjCInterop:konanRun --filter=sumComplex -PcompilerArgs="-g -Xbinary=gc=nstw -Xbinary=stripDebugInfoFromNativeLibs=false" && popd

./gradlew linux_x64PlatformLibs && pushd kotlin-native/performance && ../../gradlew :konanRun -PcompilerArgs="-g -Xbinary=gc=nstw -Xbinary=stripDebugInfoFromNativeLibs=false" && popd

### benchmark analyzer 실행.
/Volumes/WorkSpace/kotlin-nstw/kotlin-native/tools/benchmarksAnalyzer/build/bin/linuxX64/benchmarksAnalyzerReleaseExecutable/benchmarksAnalyzer.kexe z_rtgc/benchmark-results/experimental-09-27.json z_rtgc/benchmark-results/nstw-LAZY_GC-10-20.json 

### NO_FRAME vs LAZY_GC
Ring::String.stringConcat                         34544.0700 ± 58.9272     183001.9888 ± 17173.9773 -79.13
Ring::String.stringConcatNullable                 35021.0611 ± 176.4440    196198.4718 ± 33195.8759 -78.41
Startup::Singleton.initializeNested               1070.8998 ± 36.7019      2935.3192 ± 66.1246      -61.40

### Experimental vs LAZY_GC.
Ring::GraphSolver.solve *                         193309.8197 ± 469.1270   2408668.2464 ± 15585.3231-91.90
Startup::Singleton.initializeNested               463.9809 ± 13.2072       2935.3192 ± 66.1246      -83.37
Ring::Lambda.mutatingLambdaNoInline               399.4815 ± 3.6080        1956.1766 ± 111.6018     -78.15
Ring::ComplexArrays.outerProduct                  223177.1505 ± 612.5983   941508.3030 ± 3956.3755  -76.13
Ring::Euler.problem2 *                            1.8202 ± 0.0116          7.7849 ± 0.2119          -75.81

### Experimental vs NO_FRMAE.
Ring::GraphSolver.solve *                         193309.8197 ± 469.1270   6307679.6142 ± 70899.0410-96.89
Ring::ForLoops.arrayLoop                          8.9016 ± 0.0490          132.4456 ± 0.1584        -93.23
Ring::ForLoops.arrayIndicesLoop                   9.6822 ± 0.1098          137.3754 ± 0.1096        -92.87
Ring::Richards                                    185.7117 ± 3.9004        1172.1360 ± 1.9195       -83.80
Ring::Euler.problem2 *                            1.8202 ± 0.0116          8.9966 ± 0.0101          -79.62

```


## logs
```sh
-Xruntime-logs=gc=info
```

## debugging
- local.properties 파일에 아래 내용 추가.
```sh
kotlin.native.isNativeRuntimeDebugInfoEnabled=true
```

- debugging 용 test 빌드.
```sh
./gradlew -Ptest_flags="-g -Xbinary=gc=nstw  -Xbinary=stripDebugInfoFromNativeLibs=false" :kotlin-native:native:native.tests:cycle_detector
```

- vscode codeLLDB plugin 설치 및 .vscode/launch.json 설정.
* lldb.displayFormat -> hex
* lldb.dereferencePointers -> off
* lldb.lauch.expression -> native (watch 모드 evaluation 설정)

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Launch Main",
            "type": "lldb", // codeLLDB 사용
            "request": "launch",
            "program": "${workspaceRoot}/test.output/local/cycle_detector/linux_x64/cycle_detector.kexe",
            "cwd": "${workspaceRoot}",
            "env": { // Macosx 용 malloc 최적화 허용 -> (원래 기본값이 '1'이나 vscode 가 debugging 을 위해 off 함.)
                "MallocNanoZone": "1"
            }
        },
    ]
}
```
- check debug-symbol available
```sh
dsymutil --dump-debug-map test.output/local/cycle_detector/linux_x64/cycle_detector.kexe
```
