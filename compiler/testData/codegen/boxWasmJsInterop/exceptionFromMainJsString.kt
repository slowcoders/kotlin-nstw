// TARGET_BACKEND: WASM

// FILE: main.kt
fun throwSomeJsPrimitive(): Int = js("{ throw 'Test'; }")

fun main() {
    throwSomeJsPrimitive()
}

fun box() = "OK"

// FILE: entry.mjs
let nothrow = false;
try {
    const m = await import("./index.mjs")
    nothrow = true
} catch(e) {
    if (typeof e !== "string") {
        throw Error("Expected string")
    }
    if (e !== "Test") {
        throw Error("Wrong value")
    }
}
if (nothrow) throw Error("Unexpected successful call");