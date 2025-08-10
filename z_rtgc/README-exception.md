#### 최종적인 throw 함수
- ExceptionObjHolder::Throw 호출 후
- Frame 의 내용을 decrementsQ 에 저장.

### LeaveFrame 을 이용한 stackRef clearing. -> 불가.
- 일반 kotlin 함수 컴파일 시, needCleanupLandingpadAndLeaveFrame 값이 false 이다.
- 이에, Exception 발생 시 LeaveFrame 호출이 자동으로 이뤄지지 않는다.

### SetCurrentFrame + ~ExceptionObjHolderImpl 조합
1) SetCurrentFrame 호출 시 catchedFrame 을 변경하고 ~ExceptionObjHolderImpl 에서 decrementsQ 처리    
   ExceptionObjHolderImpl destructor 호출 시점이 실제 Catch 문과 일치하지 않음
   -> C++ exception 캐치 후, kotlin-exception 과 일치하지 않으면 rethrow 하는 형태로 컴파일 됨.
2) CheckCurrentFrame (Exception catch 시에만 호출됨).
   --> UnhandledException 에 대하여 별도 처리 필요.

```kotlin
CodeGenerator.kt
    fun filteringExceptionHandler(...) {
            appendingTo(forwardKotlinExceptionBlock) {
                // Rethrow Kotlin exception to real handler.
                outerHandler.genThrow(this, extractKotlinException(landingpad))
            }
    }
```

## 참고) ExceptionStack 사용하지 않으려면..
- setCurrentFrame 호출 전에 sp 를 변경하고, 호출 후 복구할 수 있으면 해결 가능
- llvm.stacksave(), llvm.stackresore() 참고할 것. -> saveStack, restoreStack
```kotlin
FunctionGenerationContext:
    internal fun epilogue() {
        ...
        appendingTo(stackLocalsInitBb) {
            if (false && context.memoryModel == MemoryModel.RTGC && needCleanupLandingpadAndLeaveFrame) {
                // preserve rollback stack
                stackLocalsManager.allocArray(context.irBuiltIns.primitiveArrayForType.getValue(context.irBuiltIns.longType).owner, Int32(32).llvm);
            }
        }
    }
```

