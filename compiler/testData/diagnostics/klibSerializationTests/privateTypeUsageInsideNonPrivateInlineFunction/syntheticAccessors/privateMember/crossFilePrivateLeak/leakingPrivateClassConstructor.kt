// LANGUAGE: +ForbidExposureOfPrivateTypesInNonPrivateInlineFunctionsInKlibs
// DIAGNOSTICS: -NOTHING_TO_INLINE

// FILE: A.kt
private class Private {
    fun foo() = "OK"
}

internal inline fun internalInlineFun(): String {
    return Private().foo()
}

// FILE: main.kt
fun box(): String {
    return internalInlineFun()
}
