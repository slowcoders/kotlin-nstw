/*
 * Copyright 2010-2024 JetBrains s.r.o. and Kotlin Programming Language contributors.
 * Use of this source code is governed by the Apache 2.0 license that can be found in the license/LICENSE.txt file.
 */

package org.jetbrains.kotlin.sir.util

import org.jetbrains.kotlin.sir.*
import org.jetbrains.kotlin.sir.builder.buildImport

val SirCallable.allParameters: List<SirParameter>
    get() = when (this) {
        is SirFunction -> this.parameters
        is SirInit -> this.parameters
        is SirSetter -> listOf(SirParameter(parameterName = parameterName, type = this.valueType))
        is SirGetter -> listOf()
    }

val SirCallable.returnType: SirType
    get() = when (this) {
        is SirFunction -> this.returnType
        is SirGetter -> this.valueType
        is SirSetter, is SirInit -> SirStructType(SirSwiftModule.void)
    }

val SirAccessor.valueType: SirType
    get() = this.parent.let {
        when (it) {
            is SirVariable -> it.type
            else -> error("Invalid accessor parent $parent")
        }
    }

val SirVariable.accessors: List<SirAccessor>
    get() = listOfNotNull(
        getter,
        setter,
    )

val SirParameter.name: String? get() = parameterName ?: argumentName

val SirType.isVoid: Boolean get() = this is SirStructType && this.declaration == SirSwiftModule.void

fun <T : SirDeclaration> SirMutableDeclarationContainer.addChild(producer: () -> T): T {
    val child = producer()
    child.parent = this
    declarations += child
    return child
}

/**
 * Add imports of [newImports] that are not yet added.
 */
fun SirModule.updateImports(newImports: List<String>) {
    val existingImports = allImports().map { it.moduleName }
    val delta = newImports.filterNot { it in existingImports }
    delta
        .filter { it != "Swift" && it != this.name }
        .forEach {
            addChild {
                buildImport {
                    moduleName = it
                }
            }
        }
}