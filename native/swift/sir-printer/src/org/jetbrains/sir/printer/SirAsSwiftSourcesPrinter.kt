/*
 * Copyright 2010-2023 JetBrains s.r.o. and Kotlin Programming Language contributors.
 * Use of this source code is governed by the Apache 2.0 license that can be found in the license/LICENSE.txt file.
 */

package org.jetbrains.sir.printer

import org.jetbrains.kotlin.sir.*
import org.jetbrains.kotlin.sir.util.Comparators
import org.jetbrains.kotlin.sir.util.swiftName
import org.jetbrains.kotlin.utils.IndentingPrinter
import org.jetbrains.kotlin.utils.SmartPrinter
import org.jetbrains.kotlin.utils.withIndent

public class SirAsSwiftSourcesPrinter(
    private val printer: SmartPrinter,
    private val stableDeclarationsOrder: Boolean,
    private val renderDocComments: Boolean,
    private val emptyBodyStub: SirFunctionBody
) : IndentingPrinter by printer {

    public companion object {

        private val fatalErrorBodyStub: SirFunctionBody = SirFunctionBody(
            listOf("fatalError()")
        )

        public fun print(
            module: SirModule,
            stableDeclarationsOrder: Boolean,
            renderDocComments: Boolean,
            emptyBodyStub: SirFunctionBody = fatalErrorBodyStub
        ): String {
            val childrenPrinter = SirAsSwiftSourcesPrinter(
                SmartPrinter(StringBuilder()),
                stableDeclarationsOrder = stableDeclarationsOrder,
                renderDocComments = renderDocComments,
                emptyBodyStub = emptyBodyStub,
            )
            val declarationsString = with(childrenPrinter) {
                module.printChildren()
                toString().trimIndent()
            }
            val importsString = if (module.imports.isNotEmpty()) {
                // We print imports after module declarations as they might lazily add new imports.
                val importsPrinter = SirAsSwiftSourcesPrinter(
                    SmartPrinter(StringBuilder()),
                    stableDeclarationsOrder = stableDeclarationsOrder,
                    renderDocComments = renderDocComments,
                    emptyBodyStub = emptyBodyStub,
                )
                with(importsPrinter) {
                    module.printImports()
                    println()
                    toString().trimIndent()
                }
            } else ""
            return importsString + declarationsString
        }
    }

    private fun SirModule.printImports() {
        imports
            .let {
                if (stableDeclarationsOrder)
                    imports.sortedWith(compareBy(
                            { it.moduleName },
                            { it.mode },
                        ))
                else
                    imports
            }.takeIf {
                it.isNotEmpty()
            }?.forEach {
                it.print()
            }?.also {
                println()
            }
    }

    private fun SirTypealias.print() {
        printDocumentation()
        printAttributes()
        printVisibility()
        print("typealias ")
        printName()
        print(" = ")
        println(type.swiftRender)
    }

    private fun SirDeclarationContainer.print() {
        if (this is SirDeclaration) {
            printDocumentation()
            printAttributes()
            if (this is SirClass) {
                printModifiers()
            } else {
                printVisibility()
            }
        }

        printContainerKeyword()
        print(" ")
        printName()
        print(" ")
        if (this is SirClass) {
            printSuperClass()
        }
        println("{")
        withIndent {
            printChildren()
        }
        println("}")
    }

    private fun SirDeclaration.printAttributes() {
        attributes.forEach {
            println("@", it.identifier, it.arguments?.joinToString(prefix = "(", postfix = ")") { parameter ->
                parameter.name?.let { "$it: ${parameter.expression}" } ?: "${parameter.expression}"
            } ?: "")
        }
    }

    private fun SirDeclarationContainer.printChildren() {
        allNonPackageEnums()
            .sortedWithIfNeeded(Comparators.stableNamedComparator)
            .forEach { it.print() }
        allTypealiases()
            .sortedWithIfNeeded(Comparators.stableNamedComparator)
            .forEach { it.print() }
        allClasses()
            .sortedWithIfNeeded(Comparators.stableNamedComparator)
            .forEach { it.print() }
        allVariables()
            .sortedWithIfNeeded(Comparators.stableVariableComparator)
            .forEach { it.print() }
        allCallables()
            .sortedWithIfNeeded(Comparators.stableCallableComparator)
            .forEach { it.print() }
        if (this is SirModule) {
            allExtensions()
                .sortedWithIfNeeded(Comparators.stableExtensionComparator)
                .forEach { it.print() }
        }
        allPackageEnums()
            .sortedWithIfNeeded(Comparators.stableNamedComparator)
            .forEach { it.print() }
    }

    private inline fun <reified T : SirElement> Sequence<T>.sortedWithIfNeeded(comparator: Comparator<in T>): Sequence<T> =
        if (stableDeclarationsOrder) sortedWith(comparator) else this

    private fun SirVariable.print() {
        printDocumentation()
        printAttributes()
        printModifiers()
        printOverride()
        print(
            "var ",
            name.swiftIdentifier,
            ": ",
            type.swiftRender,
        )
        println(" {")
        withIndent {
            getter.print()
            setter?.print()
        }
        println("}")
    }

    private fun SirCallable.print() {
        printDocumentation()
        printAttributes()
        printModifiers()
        printOverride()
        printPreNameKeywords()
        printName()
        printPostNameKeywords()
        if (this !is SirAccessor) {
            print("(")
        }
        collectParameters().print()
        if (this !is SirAccessor) {
            print(")")
        }
        printReturnType()
        println(" {")
        withIndent {
            body.print()
        }
        println("}")
    }

    private fun SirClassMemberDeclaration.printOverride() {
        if (this.isOverride) {
            print("override ")
        }
    }

    private fun SirCallable.printOverride() {
        when (this) {
            is SirInit -> if (this.isOverride && !this.isRequired) {
                print("override ")
            }
            is SirClassMemberDeclaration -> (this as SirClassMemberDeclaration).printOverride()
            else -> {}
        }
    }

    private fun SirDeclaration.printDocumentation() {
        if (!renderDocComments) return
        documentation?.lines()?.forEach { println(it.trimIndent()) }
    }

    private fun SirImport.print() {
        print(
            when (mode) {
                SirImport.Mode.Exported -> "@_exported "
                SirImport.Mode.ImplementationOnly -> "@_implementationOnly "
                null -> ""
            }
        )
        println("import $moduleName")
    }

    private fun SirDeclarationContainer.printContainerKeyword() = print(
        when (this@printContainerKeyword) {
            is SirClass -> "class"
            is SirEnum -> "enum"
            is SirExtension -> "extension"
            is SirStruct -> "struct"
            is SirModule -> error("there is no keyword for module. Do not print module as declaration container.")
        }
    )

    private fun SirClass.printSuperClass() = print(
        superClass?.let { ": ${it.swiftRender} " } ?: ""
    )

    private fun SirElement.printName() = print(
        when (this@printName) {
            is SirNamed -> name
            is SirExtension -> extendedType.swiftRender
            else -> error("There is no printable name for SirElement: ${this@printName}")
        }
    )

    private fun SirDeclaration.printVisibility() = print(
        visibility
            .takeUnless { this is SirAccessor }
            .takeIf { it != SirVisibility.INTERNAL }
            ?.let { "${it.swift} " }
            ?: ""
    )

    private fun SirClassMemberDeclaration.printModifiers() {
        when (effectiveModality) {
            SirModality.OPEN -> {
                if (visibility == SirVisibility.PUBLIC) {
                    print("open ")
                } else {
                    // Swift properties and methods are internally inheritable
                    // by default – no need to print "open"
                    printVisibility()
                }
                if (callableKind == SirCallableKind.CLASS_METHOD) {
                    print("class ")
                }
            }
            SirModality.FINAL -> {
                printVisibility()
                if (callableKind == SirCallableKind.CLASS_METHOD) {
                    print("static ")
                } else if (callableKind != SirCallableKind.FUNCTION) {
                    // to reduce noise we don't print 'final' when it's implied
                    if ((parent as? SirClass)?.modality != SirModality.FINAL) {
                        print("final ")
                    }
                }
            }
            SirModality.UNSPECIFIED -> {
                printVisibility()
                if (callableKind == SirCallableKind.CLASS_METHOD) {
                    print("class ")
                }
            }
        }
    }

    private fun SirClass.printModifiers() {
        when (modality) {
            SirModality.OPEN -> {
                if (visibility == SirVisibility.PUBLIC) {
                    print("open ")
                } else {
                    // Swift classes are internally inheritable
                    // by default – no need to print "open"
                    printVisibility()
                }
            }
            SirModality.FINAL -> {
                printVisibility()
                print("final ")
            }
            SirModality.UNSPECIFIED -> {
                printVisibility()
            }
        }
    }

    private fun SirDeclaration.printModifiers() {
        if (this is SirClassMemberDeclaration) {
            printModifiers()
        } else if (this is SirClass) {
            printModifiers()
        } else {
            printVisibility()
        }
    }

    private fun SirCallable.printPreNameKeywords() = this.also {
        when (this) {
            is SirInit -> {
                if (isRequired) {
                    print("required ")
                }
                if (isConvenience) {
                    print("convenience ")
                }
            }
            is SirFunction -> {}
            is SirGetter -> print("get")
            is SirSetter -> print("set")
        }
    }

    private fun SirCallable.printName() = print(
        when (this) {
            is SirInit -> "init"
            is SirFunction -> "func $name"
            is SirGetter,
            is SirSetter,
                -> ""
        }
    )

    private fun SirCallable.printPostNameKeywords() = when (this) {
        is SirInit -> "?".takeIf { isFailable }?.let { print(it) }
        is SirFunction,
        is SirGetter,
        is SirSetter,
            -> print("")
    }

    private fun SirCallable.collectParameters(): List<SirParameter> = when (this) {
        is SirGetter -> emptyList()
        is SirSetter -> emptyList()
        is SirFunction -> listOfNotNull(extensionReceiverParameter) + parameters
        is SirInit -> parameters
    }

    private fun SirCallable.printReturnType() = print(
        when (this) {
            is SirFunction -> " -> ${returnType.swiftRender}"
            is SirInit,
            is SirGetter,
            is SirSetter,
                -> ""
        }
    )

    private fun List<SirParameter>.print() =
        takeIf { it.isNotEmpty() }
            ?.let {
                println()
                withIndent {
                    this.forEachIndexed { index, sirParameter ->
                        print(sirParameter.swiftRender)
                        if (index != lastIndex) {
                            println(",")
                        } else {
                            println()
                        }
                    }
                }
            }

    private fun SirFunctionBody?.print() = (this ?: emptyBodyStub)
        .statements
        .forEach {
            println(it)
        }
}

private val SirVisibility.swift
    get(): String = when (this) {
        SirVisibility.PRIVATE -> "private"
        SirVisibility.FILEPRIVATE -> "fileprivate"
        SirVisibility.INTERNAL -> "internal"
        SirVisibility.PUBLIC -> "public"
        SirVisibility.PACKAGE -> "package"
    }

private val simpleIdentifierRegex = Regex("[_a-zA-Z][_a-zA-Z0-9]*")

private val String.swiftIdentifier get() = if (simpleIdentifierRegex.matches(this)) this else "`$this`"

private val SirParameter.swiftRender
    get(): String = (argumentName ?: "_") +
            (parameterName?.let { " $it" } ?: "") + ": " +
            type.swiftRender

private val SirType.swiftRender: String
    get() = when (this) {
        is SirOptionalType -> wrappedType.swiftRender + "?"
        is SirArrayType -> "[${elementType.swiftRender}]"
        is SirDictionaryType -> "[${keyType.swiftRender}: ${valueType.swiftRender}]"
        else -> swiftName
    }

private val SirClassMemberDeclaration.callableKind: SirCallableKind
    get() = when (this) {
        is SirVariable -> kind
        is SirCallable -> (this as SirCallable).kind
    }
