// ISSUE: KT-75061
// LANGUAGE: +ContextSensitiveResolutionUsingExpectedType

sealed interface MySealed {
    class Left(val x: String): MySealed
    class Right(val y: String): MySealed
    class NonSealedSubclass(val z: String)
}

fun MySealed.getOrElse() = when (this) {
    is Left -> x
    is Right -> y
    is NonSealedSubclass -> z
}
