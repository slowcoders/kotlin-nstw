/*
 * Copyright 2010-2021 JetBrains s.r.o. Use of this source code is governed by the Apache 2.0 license
 * that can be found in the LICENSE file.
 */
// KT-66100: AssertionError: Expected an exception of class IndexOutOfBoundsException to be thrown, but was completed successfully.
// IGNORE_BACKEND: JS, JS_ES6
// WITH_STDLIB
import kotlin.test.*

fun box(): String {
    val array = Array(10) { 0L }
    val array1 = Array(3) { 0L }
    var j = 8

    assertFailsWith<IndexOutOfBoundsException> {
        for (i in array.indices step 2) {
            array[j] = 6
            j++
        }
    }

    assertFailsWith<IndexOutOfBoundsException> {
        for (i in array.indices step 2) {
            array[i - 1] = 6
        }
    }

    assertFailsWith<IndexOutOfBoundsException> {
        for (i in array.indices step 2) {
            array1[i] = 6
        }
    }
    return "OK"
}
