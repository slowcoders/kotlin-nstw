// TARGET_BACKEND: JS
// FIR_IDENTICAL
fun testArrayAccess1(d: dynamic) = d["KEY"]

fun testArrayAccess2(d: dynamic) = d()["KEY"]

fun testArrayAccess3(d: dynamic) = d.get("KEY")
