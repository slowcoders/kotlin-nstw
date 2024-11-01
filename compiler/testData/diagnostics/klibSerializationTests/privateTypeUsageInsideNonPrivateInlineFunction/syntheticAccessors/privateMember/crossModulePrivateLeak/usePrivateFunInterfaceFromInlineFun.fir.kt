// LANGUAGE: +ForbidExposureOfPrivateTypesInNonPrivateInlineFunctionsInKlibs
// DIAGNOSTICS: -NOTHING_TO_INLINE

// MODULE: lib
// FILE: I.kt
private fun interface I {
    fun foo(): Int
}

inline fun publicInlineFun(): Int = (<!IR_PRIVATE_TYPE_USED_IN_NON_PRIVATE_INLINE_FUNCTION_ERROR!>I { 1 }<!>).<!NON_PUBLIC_CALL_FROM_PUBLIC_INLINE!>foo<!>()

internal inline fun internalInlineFun(): Int = (<!IR_PRIVATE_TYPE_USED_IN_NON_PRIVATE_INLINE_FUNCTION_ERROR!>I { 1 }<!>).<!PRIVATE_CLASS_MEMBER_FROM_INLINE!>foo<!>()

// MODULE: main()(lib)
// FILE: main.kt
fun box(): String {
    var result = 0
    result += publicInlineFun()
    result += internalInlineFun()
    return if (result == 2) "OK" else result.toString()
}
