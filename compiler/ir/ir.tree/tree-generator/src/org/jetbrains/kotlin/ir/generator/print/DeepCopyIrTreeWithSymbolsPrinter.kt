/*
 * Copyright 2010-2024 JetBrains s.r.o. and Kotlin Programming Language contributors.
 * Use of this source code is governed by the Apache 2.0 license that can be found in the license/LICENSE.txt file.
 */

package org.jetbrains.kotlin.ir.generator.print

import org.jetbrains.kotlin.descriptors.ValueClassRepresentation
import org.jetbrains.kotlin.generators.tree.*
import org.jetbrains.kotlin.generators.tree.imports.ArbitraryImportable
import org.jetbrains.kotlin.generators.tree.printer.*
import org.jetbrains.kotlin.ir.generator.IrTree
import org.jetbrains.kotlin.ir.generator.deepCopyTypeRemapperType
import org.jetbrains.kotlin.ir.generator.elementTransformerVoidType
import org.jetbrains.kotlin.ir.generator.irImplementationDetailType
import org.jetbrains.kotlin.ir.generator.irSimpleTypeType
import org.jetbrains.kotlin.ir.generator.irTypeType
import org.jetbrains.kotlin.ir.generator.model.DeepCopyConstructorPolicy
import org.jetbrains.kotlin.ir.generator.model.Element
import org.jetbrains.kotlin.ir.generator.model.Field
import org.jetbrains.kotlin.ir.generator.model.Implementation
import org.jetbrains.kotlin.ir.generator.model.ListField
import org.jetbrains.kotlin.ir.generator.model.symbol.symbolRemapperMethodName
import org.jetbrains.kotlin.ir.generator.symbolRemapperType
import org.jetbrains.kotlin.ir.generator.typeRemapperType
import org.jetbrains.kotlin.utils.withIndent

internal class DeepCopyIrTreeWithSymbolsPrinter(
    printer: ImportCollectingPrinter,
    override val visitorType: ClassRef<*>,
) : AbstractVisitorPrinter<Element, Field>(printer) {
    override val visitorTypeParameters: List<TypeVariable>
        get() = emptyList()

    override val visitorDataType: TypeRef
        get() = StandardTypes.nothing.copy(nullable = true)

    override fun visitMethodReturnType(element: Element): TypeRef = element

    override val visitorSuperTypes: List<ClassRef<PositionTypeParameterRef>>
        get() = listOf(elementTransformerVoidType)

    override val optIns: List<ClassRef<*>> = listOf(irImplementationDetailType)

    override val implementationKind: ImplementationKind
        get() = ImplementationKind.OpenClass

    private val symbolRemapperParameter = PrimaryConstructorParameter(
        FunctionParameter("symbolRemapper", symbolRemapperType),
        VariableKind.VAL,
        Visibility.PRIVATE
    )

    private val typeRemapperParameter = PrimaryConstructorParameter(
        FunctionParameter("typeRemapper", typeRemapperType.copy(nullable = true), "null"),
        VariableKind.PARAMETER
    )

    override val constructorParameters: List<PrimaryConstructorParameter> = listOf(symbolRemapperParameter, typeRemapperParameter)

    override fun ImportCollectingPrinter.printAdditionalMethods() {
        addImport(ArbitraryImportable("org.jetbrains.kotlin.utils", "memoryOptimizedMap"))

        printPropertyDeclaration(
            name = "transformedModule",
            type = IrTree.moduleFragment.copy(nullable = true),
            kind = VariableKind.VAR,
            visibility = Visibility.PRIVATE,
            initializer = "null",
        )
        println()
        printPropertyDeclaration(
            name = "typeRemapper",
            type = typeRemapperType,
            kind = VariableKind.VAL,
            visibility = Visibility.PRIVATE,
            initializer = "${typeRemapperParameter.name} ?: ${deepCopyTypeRemapperType.render()}(${symbolRemapperParameter.name})",
        )
        println()
        println()
        printInitBlock()
        println()
        printUtils()
    }

    private fun ImportCollectingPrinter.printInitBlock() {
        print("init")
        printBlock {
            println("// TODO refactor")
            println("// After removing usages of ${deepCopyTypeRemapperType.simpleName} constructor from compose, the lateinit property `${deepCopyTypeRemapperType.simpleName}.deepCopy`")
            println("// can be refactored to a constructor parameter.")
            print("(this.${typeRemapperParameter.name} as? ${deepCopyTypeRemapperType.render()})?.let")
            printBlock {
                println("it.deepCopy = this")
            }
        }
    }

    override fun printMethodsForElement(element: Element) {
        if (!element.generateVisitorMethod) return

        printer.run {
            if (element.isRootElement) {
                println()
                printVisitMethodDeclaration(element, hasDataParameter = false, override = true, returnType = element)
                println(" =")
                withIndent {
                    println("throw IllegalArgumentException(\"Unsupported element type: $${element.visitorParameterName}\")")
                }
                return
            }

            if (element.implementations.isEmpty()) return

            println()
            printVisitMethodDeclaration(element, hasDataParameter = false, override = true, returnType = element)

            println(" =")
            withIndent {
                val implementation = element.implementations.singleOrNull() ?: error("Ambiguous implementation")
                print(implementation.render())
                if (useWithShapeConstructor(element)) {
                    print("WithShape")
                }
                val constructorArguments: List<Field> =
                    implementation.constructorArguments()
                println("(")
                withIndent {
                    for (field in constructorArguments) {
                        if (element.isSubclassOfAny(
                                IrTree.delegatingConstructorCall,
                                IrTree.enumConstructorCall
                            ) && field.name == "origin"
                        ) continue

                        print(field.name, " = ")
                        copyField(element, field)
                        println(",")
                    }
                    if (element.isSubclassOfAny(
                            IrTree.blockBody,
                            IrTree.constantObject,
                            IrTree.constantArray,
                            IrTree.expressionBody,
                            IrTree.constantPrimitive
                        )
                    ) {
                        println("constructorIndicator = null")
                    }
                    if (useWithShapeConstructor(element)) {
                        printWithShapeExtraArguments(element)
                    }
                }
                val fieldsInApply = implementation.fieldsInBody.filter { !it.deepCopyExcludeFromApply && it !in constructorArguments }
                printApply(element, fieldsInApply)
            }
        }
    }

    private fun ImportCollectingPrinter.copyField(element: Element, field: Field) {
        if (field is ListField) {
            print(element.visitorParameterName, ".", field.name, field.call(), "memoryOptimizedMap")
            print(" { ")
            copyValue(field, "it")
            print(" }")
        } else {
            copyValue(field, element.visitorParameterName, ".", field.name)
            if (element.isSubclassOf(IrTree.property) && field.name in setOf("backingField", "getter", "setter")) {
                print("?.also { it.correspondingPropertySymbol = symbol }")
            }
        }
    }

    private fun ImportCollectingPrinter.copyValue(field: Field, vararg valueArgs: Any?) {
        val typeRef = if (field is ListField) {
            field.baseType
        } else {
            field.typeRef
        }
        val symbolFieldClass = field.symbolClass
        val safeCall = if (typeRef.nullable) "?." else "."
        when {
            typeRef !is ClassOrElementRef -> {
                print(*valueArgs)
            }
            symbolFieldClass != null -> {
                val symbolRemapperFunction =
                    symbolRemapperMethodName(symbolFieldClass, field.symbolFieldRole ?: AbstractField.SymbolFieldRole.REFERENCED)
                if (typeRef.nullable) {
                    print(*valueArgs, "?.let(", symbolRemapperParameter.name, "::", symbolRemapperFunction, ")")
                } else {
                    print(symbolRemapperParameter.name, ".", symbolRemapperFunction, "(", *valueArgs, ")")
                }
            }
            typeRef.isSameClassAs(IrTree.loop) -> {
                print("transformedLoops.getOrDefault(", *valueArgs, ", ", *valueArgs, ")")
            }
            typeRef is ElementOrRef<*> -> {
                print(*valueArgs, safeCall, "transform()")
            }
            typeRef.isSameClassAs(irTypeType) -> {
                print(*valueArgs, safeCall, "remapType()")
            }
            typeRef.isSameClassAs(type<ValueClassRepresentation<*>>()) -> {
                addImport(irSimpleTypeType)
                print(*valueArgs, safeCall, "mapUnderlyingType { it.remapType() as IrSimpleType }")
            }
            typeRef is ClassRef<*> -> {
                print(*valueArgs)
            }
        }
    }

    private fun Implementation.constructorArguments(): List<Field> {
        val filteredConstructorFields = fieldsInConstructor
            .filter { it.deepCopyExcludeFromConstructor != DeepCopyConstructorPolicy.NEVER_INCLUDED }
        val filteredBodyFields = fieldsInBody
            .filter { it.deepCopyExcludeFromConstructor == DeepCopyConstructorPolicy.ALWAYS_INCLUDED }
        val allFields = filteredConstructorFields + filteredBodyFields
        return allFields
    }

    private fun useWithShapeConstructor(element: Element): Boolean =
        element.isSubclassOfAny(IrTree.functionAccessExpression, IrTree.functionReference, IrTree.propertyReference)

    private fun ImportCollectingPrinter.printApply(element: Element, applyFields: List<Field>) {
        if (element.hasEmptyApplyBody()) {
            println(")")
        } else {
            print(").apply")
            printBlock {
                if (element.isSubclassOf(IrTree.declaration) && !element.isSubclassOf(IrTree.variable)) {
                    println("with(factory) { declarationCreated() }")
                }
                if (element.isSubclassOf(IrTree.loop)) {
                    println("transformedLoops[", element.visitorParameterName, "] = this")
                }
                if (element.isSubclassOf(IrTree.moduleFragment)) {
                    println("this@DeepCopyIrTreeWithSymbols.transformedModule = this")
                }
                for (field in applyFields) {
                    if (element.isSubclassOf(IrTree.constructor) && field.name == "contextReceiverParametersCount") continue
                    if (element.isSubclassOf(IrTree.script) && field.name == "annotations") continue

                    when {
                        field.isMutable -> {
                            print(field.name, " = ")
                            copyField(element, field)
                            println()
                        }
                        field is ListField && field.mutability == ListField.Mutability.MutableList -> {
                            print(field.name, ".addAll(")
                            copyField(element, field)
                            println(")")
                        }
                    }
                }
                if (element.isSubclassOf(IrTree.attributeContainer)) {
                    println("processAttributes(", element.visitorParameterName, ")")
                }
                if (element.isSubclassOf(IrTree.memberAccessExpression) && !element.isSubclassOf(IrTree.localDelegatedPropertyReference)) {
                    println("copyRemappedTypeArgumentsFrom(", element.visitorParameterName, ")")
                    println("transformValueArguments(", element.visitorParameterName, ")")
                }
                if (element.isSubclassOf(IrTree.function)) {
                    println("valueParameters = ${element.visitorParameterName}.valueParameters.memoryOptimizedMap { it.transform() }")
                }
                if (element.isSubclassOf(IrTree.file)) {
                    println("module = transformedModule ?: ${element.visitorParameterName}.module")
                }
                if (element.isSubclassOf(IrTree.moduleFragment)) {
                    println("this@DeepCopyIrTreeWithSymbols.transformedModule = null")
                }
                if (element.isSubclassOf(IrTree.script)) {
                    printScriptApply(element)
                }
            }
        }
    }

    private fun Element.hasEmptyApplyBody() = isSubclassOfAny(
        IrTree.branch,
        IrTree.syntheticBody,
        IrTree.catch,
        IrTree.spreadElement,
        IrTree.suspendableExpression,
        IrTree.suspensionPoint,
        IrTree.expressionBody
    )

    private fun ImportCollectingPrinter.printWithShapeExtraArguments(element: Element) {
        println("typeArgumentsCount = ${element.visitorParameterName}.typeArgumentsCount,")
        println("hasDispatchReceiver = ${element.visitorParameterName}.targetHasDispatchReceiver,")
        println("hasExtensionReceiver = ${element.visitorParameterName}.targetHasExtensionReceiver,")
        if (!element.isSubclassOf(IrTree.propertyReference)) {
            println("valueArgumentsCount = ${element.visitorParameterName}.valueArgumentsCount,")
            println("contextParameterCount = ${element.visitorParameterName}.targetContextParameterCount,")
        }
    }

    private fun ImportCollectingPrinter.printScriptApply(element: Element) {
        println("importedScripts = ${element.visitorParameterName}.importedScripts")
        println("resultProperty = ${element.visitorParameterName}.resultProperty")
        println("earlierScripts = ${element.visitorParameterName}.earlierScripts")
        println("earlierScriptsParameter = ${element.visitorParameterName}.earlierScriptsParameter")
    }

    private fun ImportCollectingPrinter.printUtils() {
        printlnMultiLine(
            """
            protected open fun <D : IrAttributeContainer> D.processAttributes(other: IrAttributeContainer?): D =
                copyAttributes(other)
        
            protected inline fun <reified T : IrElement> T.transform() =
                transform(this@DeepCopyIrTreeWithSymbols, null) as T
        
            protected fun ${irTypeType.render()}.remapType() = typeRemapper.remapType(this)
        
            protected fun IrMutableAnnotationContainer.transformAnnotations(declaration: IrAnnotationContainer) {
                annotations = declaration.annotations.memoryOptimizedMap { it.transform() }
            }
        
            protected fun IrMemberAccessExpression<*>.copyRemappedTypeArgumentsFrom(other: IrMemberAccessExpression<*>) {
                assert(typeArgumentsCount == other.typeArgumentsCount) {
                    "Mismatching type arguments: ${'$'}typeArgumentsCount vs ${'$'}{other.typeArgumentsCount} "
                }
                for (i in 0 until typeArgumentsCount) {
                    putTypeArgument(i, other.getTypeArgument(i)?.remapType())
                }
            }
        
            protected fun <T : IrMemberAccessExpression<*>> T.transformReceiverArguments(original: T): T =
                apply {
                    dispatchReceiver = original.dispatchReceiver?.transform()
                    extensionReceiver = original.extensionReceiver?.transform()
                }
        
            protected fun <T : IrMemberAccessExpression<*>> T.transformValueArguments(original: T) {
                transformReceiverArguments(original)
                for (i in 0 until original.valueArgumentsCount) {
                    putValueArgument(i, original.getValueArgument(i)?.transform())
                }
            }
        
            private val transformedLoops = HashMap<IrLoop, IrLoop>()
            """
        )
    }
}