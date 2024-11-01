// LANGUAGE: +ForbidExposureOfPrivateTypesInNonPrivateInlineFunctionsInKlibs
// DIAGNOSTICS: -NOTHING_TO_INLINE

// MODULE: lib
// FILE: A.kt
private class Private {
    fun foo() = "OK"
}

internal inline fun internalInlineFun(): String {
    return Private().foo()
}

// MODULE: main()(lib)
// FILE: main.kt
fun box(): String {
    return internalInlineFun()
}
