// TARGET_BACKEND: JVM_IR
// ISSUE: KT-72345
import java.util.Properties

internal class MyProperties : Properties()

fun box(): String {
    return "OK"
}
