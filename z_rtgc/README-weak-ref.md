### Konan_getWeakReferenceImpl(referent). WeakPrivate.kt 참조
- WeakReferenceImpl 의 구현체인 WeakReferenceCounter 생성.
- WeakReferenceCounter->referred 에 referent 를 COpaquePointer 형식(ref-count 없이) 저장.
- referent->metaObject 생성; referent->metaObject->WeakReference.counter_ = 저장;
- kotlin.native.ref.WeakReference.pointer 에 WeakReferenceCounter 저장. (strong-reachable)

### Cleaner
-  finalize 를 대신하는 객체. 