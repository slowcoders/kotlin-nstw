inline fun foo(x: Int) {
    val y = x > 23
    if (y) {
        println("foo")
    }
}

fun bar() {
    foo(42)
}

// LINES(JS): 1 1 2 3 3 3 4 4 8 8 * 2 9 2 3 3 3 4 4
