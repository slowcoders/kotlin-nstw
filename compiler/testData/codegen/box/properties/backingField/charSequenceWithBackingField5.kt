// TARGET_BACKEND: JVM_IR
// TARGET_BACKEND: JS
// TARGET_BACKEND: JS_ES6
// TARGET_BACKEND: WASM
// IGNORE_BACKEND_K1: JVM_IR, JS, JS_ES6, WASM

open class Base {
    open val x: CharSequence = "BASE"
         // field = "BASE"
}

class Ok : Base() {
    override val x: CharSequence
        internal field: String = "OK"
}

fun box(): String {
    return Ok().x
}
