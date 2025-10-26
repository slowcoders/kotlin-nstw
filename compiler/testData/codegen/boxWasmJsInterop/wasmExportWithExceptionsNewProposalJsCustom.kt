// TARGET_BACKEND: WASM
// USE_NEW_EXCEPTION_HANDLING_PROPOSAL
/// MODULE: main
// FILE: main.kt

import kotlin.wasm.WasmExport

fun throwCustomJsErrorHelper(): Int = js("""
  {
    const e = new Error("Custom error");
    e.name = "MyCustomError";
    throw e;
  }
""")

@WasmExport
fun runWithCustomJsError() {
    throwCustomJsErrorHelper()
}

fun box() = "OK"

// FILE: entry.mjs
import { runWithCustomJsError } from "./index.mjs"

let nothrow = false;
try {
    runWithCustomJsError();
    nothrow = true;
} catch (e) {
    if (!(e instanceof Error)) {
        throw Error("Expected Error");
    }
    if (e.name !== "MyCustomError") {
        throw Error("Wrong name");
    }
    if (e.message !== "Custom error") {
        throw Error("Wrong message");
    }
}
if (nothrow) throw Error("Unexpected successful call");