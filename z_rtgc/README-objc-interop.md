### ObjC 객체 생성
```cpp
- Wrapper class 생성.
    ObjCInterop.mm:289
    CreateKotlinObjCClass(const KotlinObjCClassInfo* info);
- getBackRef(id)
    KotlinWrapper class 는 내부에 'body' 라는 field 를 가지며,    
    해당 필드에 kotlin BackRefFromAssociatedObject 객체를 저장한다.
    backRef 는 objC 객체의 refCount 를 복제(?)하여, weakRef 를 구현하기 위해 시용한다.
- 객체 생성.
    ObjectiveCImpl.kt
    private fun allocObjCObject(clazz: NativePtr): NativePtr
        ObjCInterop.mm
        Kotlin_objc_allocWithZone
            id allocWithZoneImp(Class self, SEL _cmd, void* zone)
                ...
                getBackRef(result, classData)->initAndAddRef(kotlinObj);
                void BackRefFromAssociatedObject::initAndAddRef(ObjHeader* obj) 
                    ForeignRefManager* initForeignRef(ObjHeader* object)
                        addHeapRef(object);

            retainImpl()
                getBackRef(self)->addRef<ErrorPolicy::kTerminate>();
                BackRefFromAssociatedObject::addRef()
                    ForeignRefManager* initForeignRef(ObjHeader* object)
                        addHeapRef(object);

- 
collectRootSet
WaitForThreadsSuspension

```

