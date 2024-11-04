
###  Platform.kt
```
    public val memoryModel: MemoryModel
        get() = MemoryModel.values()[Platform_getMemoryModel()]
        --> RuntimeUtils.valuesForEnum 을 반복 호출. Values array 반복 생성.
```

### WorkerBoundReference
```kotlin
@GCUnsafeCall("Kotlin_Any_share")
external fun rtgc_makeShared(obj: Any)
...
public class WorkerBoundReference<out T : Any>(value: T) {
...    
    init {        
        // for Experimental mode compatibility in RTGC memory model.
        // The value(=valueBeforeFreezing) is not thread-safe until freezed.
        rtgc_makeShared(value);
    }
...
}
```

### Strict Mode - Incremental Scan. -> JVM에 적용 가능???
* Frame 의 일부만 Scan 하여 garbage 처리.
* EnterFrame 시, 현재 previous Frame 에 대한 IncrementStack
* UpdateReturnRef 는 즉각 처리.
* RememberNewContainer 함수 필요. 
```
```