// LANGUAGE: +ExpectedTypeGuidedResolution

package test

enum class Sample {
    FIRST, SECOND, THIRD;
}

fun foo(sample: Sample): Int = 1

fun bar(): Int = foo(FIRST)