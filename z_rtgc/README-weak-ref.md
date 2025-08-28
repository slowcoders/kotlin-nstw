### Konan_getWeakReferenceImpl(referent). WeakPrivate.kt 참조
- WeakReferenceImpl 의 구현체인 WeakReferenceCounter 생성.
- WeakReferenceCounter->referred 에 referent 를 COpaquePointer 형식(ref-count 없이) 저장.
- referent->metaObject 생성; referent->metaObject->WeakReference.counter_ = 저장;
- kotlin.native.ref.WeakReference.pointer 에 WeakReferenceCounter 저장. (strong-reachable)

### Cleaner
-  finalize 를 대신하는 객체. 

/Users/zeedhoon/.konan/dependencies/llvm-19-aarch64-macos-dev-79/bin/clang++ -cc1 -emit-obj -disable-llvm-passes -x ir -O1 -triple arm64-apple-macos11.0 /Users/zeedhoon/slowcoders/kotlin/kotlin-nstw/kotlin-native/runtime/build/bitcode/test/macos_arm64/macos_arm64PmcsGcTest.bc -o /Users/zeedhoon/slowcoders/kotlin/kotlin-nstw/kotlin-native/runtime/build/obj/macos_arm64/macos_arm64PmcsGcTest.o

/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/ld -demangle -dynamic -arch arm64 -platform_version macos 11.0.0 15.0 -syslibroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX15.5.sdk -o /Users/zeedhoon/slowcoders/kotlin/kotlin-nstw/kotlin-native/runtime/build/bin/test/macos_arm64/macos_arm64PmcsGcTest.kexe /Users/zeedhoon/slowcoders/kotlin/kotlin-nstw/kotlin-native/runtime/build/obj/macos_arm64/macos_arm64PmcsGcTest.o -lSystem -lc++ -lobjc -framework Foundation /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/17/lib/darwin//libclang_rt.osx.a -rpath @executable_path/../Frameworks