// LANGUAGE: +ForbidExposureOfPrivateTypesInNonPrivateInlineFunctionsInKlibs
// DIAGNOSTICS: -NOTHING_TO_INLINE

// MODULE: lib
// FILE: a.kt
private class Private

internal inline fun getPrivateKlass(): String {
    <!IR_PRIVATE_TYPE_USED_IN_NON_PRIVATE_INLINE_FUNCTION_ERROR!>val klass = <!IR_PRIVATE_TYPE_USED_IN_NON_PRIVATE_INLINE_FUNCTION_ERROR!>Private::class<!><!>
    return <!IR_PRIVATE_TYPE_USED_IN_NON_PRIVATE_INLINE_FUNCTION_ERROR!>klass<!>.simpleName ?: "null"
}

// MODULE: main()(lib)
// FILE: main.kt
fun box(): String {
    val result = getPrivateKlass()
    if (result != "Private") return result
    return "OK"
}
