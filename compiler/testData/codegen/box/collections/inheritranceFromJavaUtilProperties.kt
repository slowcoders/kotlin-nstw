// TARGET_BACKEND: JVM_IR
// JDK_KIND: FULL_JDK_11
import java.util.Properties

internal class MyProperties : Properties()

fun box(): String {
    return "OK"
}
