// IGNORE_BACKEND: JS
// IGNORE_BACKEND: JS_ES6
// IGNORE_BACKEND: WASM
// WASM_MUTE_REASON: IGNORED_IN_JS
// reason - no error from division by zero in JS or WASM

fun expectFail(f: () -> Unit): Nothing? {
    try {
        f()
    } catch (e: ArithmeticException) {
        return null
    }
    throw AssertionError("Expected ArithmeticException to be thrown")
}

fun box(): String {
    val a1 = 0
    val a2 = expectFail { 1 / 0 } ?: 0
    val a3 = expectFail { 1 / a1 } ?: 0
    val a4 = expectFail { 1 / a2 } ?: 0
    val a5 = expectFail { 2 * (1 / 0) } ?: 0
    val a6 = expectFail { 2 * 1 / 0 } ?: 0

    val s1 = expectFail { "${2 * (1 / 0) }" } ?: "OK"

    return s1
}