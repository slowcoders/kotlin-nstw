// LANGUAGE: +ForbidExposureOfPrivateTypesInNonPrivateInlineFunctionsInKlibs
// DIAGNOSTICS: -NOTHING_TO_INLINE

// MODULE: lib
// FILE: a.kt
private class Private

internal inline fun isPrivate(obj: Any): String = when (obj) {
    <!IR_PRIVATE_TYPE_USED_IN_NON_PRIVATE_INLINE_FUNCTION_ERROR!>is Private<!> -> "isPrivate"
    else -> "OK1"
}

internal inline fun asPrivate(obj: Any): String {
    try {
        <!IR_PRIVATE_TYPE_USED_IN_NON_PRIVATE_INLINE_FUNCTION_ERROR!>val privateObj = <!IR_PRIVATE_TYPE_USED_IN_NON_PRIVATE_INLINE_FUNCTION_ERROR!>obj as Private<!><!>
        return "asPrivate"
    } catch (e: ClassCastException) {
        return "OK2"
    }
}

// MODULE: main()(lib)
// FILE: main.kt
fun box(): String {
    val result = isPrivate(Any()) + asPrivate(Any())
    if (result != "OK1OK2") return result
    return "OK"
}
