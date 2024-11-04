./gradlew macos_x64PlatformLibs

function build_benchmark {
    ../../gradlew -Pkotlin.native.binary.memoryModel=$1 :konanRun
    out_dir=../../z_rtgc/benchmarks/$2
    res_dir=../../z_rtgc/benchmark-results
    rm -rf $out_dir
    mkdir -p $out_dir
    mkdir -p $res_dir
    cp ./build/nativeReport.json $res_dir/"$2-`date`".json
    cp ./build/nativeReport.json $out_dir/
    cp ./helloworld/build   $out_dir/helloworld
    cp ./objcinterop/build  $out_dir/objcinterop
    cp ./numerical/build    $out_dir/numerical
    cp ./ring/build         $out_dir/ring
    cp ./startup/build      $out_dir/startup
}

pushd kotlin-native/performance

build_benchmark rtgc         rtgc
# build_benchmark experimental experimental
# build_benchmark strict       strict
# build_benchmark rtgc         rtgc2

popd