import kotlin.reflect.KFunction1

@DslMarker
annotation class Ann

@Ann
class A

fun A.f(x: Short): Short = x
fun A.f(x: Long): Long = x

@Ann
class B

fun foo(x: A.() -> Unit) {}
fun bar(x: B.() -> Unit) {}

fun h(f: KFunction1<Int, Unit>) {}
fun A.g(x: Int, y: Int = 0) {}

fun test() {
    foo {
        bar {
            <!NONE_APPLICABLE!>f<!>(1)
            h(<!ADAPTED_CALLABLE_REFERENCE_AGAINST_REFLECTION_TYPE!>::g<!>)
        }
    }
}
