// LANGUAGE: +ForbidExposureOfPrivateTypesInNonPrivateInlineFunctionsInKlibs
// DIAGNOSTICS: -NOTHING_TO_INLINE

// FILE: I.kt
private fun interface I {
    fun foo(): Int
}

inline fun publicInlineFun(): Int = (I { 1 }).foo()

internal inline fun internalInlineFun(): Int = (I { 1 }).foo()

// FILE: main.kt
fun box(): String {
    var result = 0
    result += publicInlineFun()
    result += internalInlineFun()
    return if (result == 2) "OK" else result.toString()
}
