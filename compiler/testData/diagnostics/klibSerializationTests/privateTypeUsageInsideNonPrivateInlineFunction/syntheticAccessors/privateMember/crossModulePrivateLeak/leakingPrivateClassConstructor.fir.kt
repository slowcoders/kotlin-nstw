// LANGUAGE: +ForbidExposureOfPrivateTypesInNonPrivateInlineFunctionsInKlibs
// DIAGNOSTICS: -NOTHING_TO_INLINE

// MODULE: lib
// FILE: A.kt
private class Private {
    fun foo() = "OK"
}

internal inline fun internalInlineFun(): String {
    return <!IR_PRIVATE_TYPE_USED_IN_NON_PRIVATE_INLINE_FUNCTION_ERROR!><!PRIVATE_CLASS_MEMBER_FROM_INLINE!>Private<!>()<!>.<!PRIVATE_CLASS_MEMBER_FROM_INLINE!>foo<!>()
}

// MODULE: main()(lib)
// FILE: main.kt
fun box(): String {
    return internalInlineFun()
}
