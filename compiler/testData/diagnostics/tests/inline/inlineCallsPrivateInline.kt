// FIR_IDENTICAL
// RENDER_DIAGNOSTICS_FULL_TEXT
class AAA {
    inline fun <reified T> myFunction() {
        <!NON_PUBLIC_CALL_FROM_PUBLIC_INLINE!>localDeclarations<!>()
    }

    private <!NOTHING_TO_INLINE!>inline<!> fun localDeclarations(): Boolean {
        return true
    }
}
