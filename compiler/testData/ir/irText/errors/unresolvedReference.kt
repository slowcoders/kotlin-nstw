// IGNORE_ERRORS
// IGNORE_FIR_DIAGNOSTICS
// DIAGNOSTICS: -UNRESOLVED_REFERENCE -OVERLOAD_RESOLUTION_AMBIGUITY
// IGNORE_BACKEND_K1: JS
// SKIP_IR_SERIALIZATION
// REASON: Cannot serialize error type: ERROR CLASS: Unresolved name: unresolved

val test1 = unresolved

val test2: Unresolved =
        unresolved()

val test3 = 42.unresolved(56)
