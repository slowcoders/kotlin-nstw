// LANGUAGE: +EliminateAmbiguitiesWithExternalTypeParameters, -ProhibitOverloadingBetweenVarargsAndArrays
// WITH_STDLIB

class AllCollection<T> {
    <!CONFLICTING_OVERLOADS_DEPRECATION!>fun <K, T> addAll(vararg values: T, values2: Array<K>)<!> = "OK" // 1
    <!CONFLICTING_OVERLOADS_DEPRECATION!>fun <K, T> addAll(values: Array<K>, vararg values2: T)<!> = 1 // 2
}

fun main(c: AllCollection<Any?>) {
    // KT-49620
    c.<!OVERLOAD_RESOLUTION_AMBIGUITY!>addAll<!>(arrayOf(""), values2 = arrayOf(""))
}