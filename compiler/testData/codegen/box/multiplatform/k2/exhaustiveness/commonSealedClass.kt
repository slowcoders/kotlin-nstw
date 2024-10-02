// LANGUAGE: +MultiPlatformProjects
// TARGET_BACKEND: JVM
// ISSUE: KT-44474

// MODULE: common
// FILE: common.kt
sealed class Base()

class A : Base()
object B : Base()

fun testCommon(base: Base) {
    val x = when (base) { // must be Ok
        is A -> 1
        B -> 2
    }
}

// MODULE: platform()()(common)
// FILE: main.kt

fun testPlatform(base: Base) {
    val x = when (base) { // must be OK
        is A -> 1
        B -> 2
    }
}

fun box() = "OK"
