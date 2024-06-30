/*
 * Copyright 2010-2024 JetBrains s.r.o. and Kotlin Programming Language contributors.
 * Use of this source code is governed by the Apache 2.0 license that can be found in the license/LICENSE.txt file.
 */

package org.jetbrains.kotlin.backend.common.lower

import org.jetbrains.kotlin.backend.common.BackendContext
import org.jetbrains.kotlin.backend.common.FileLoweringPass
import org.jetbrains.kotlin.descriptors.DescriptorVisibilities
import org.jetbrains.kotlin.descriptors.Modality
import org.jetbrains.kotlin.ir.IrElement
import org.jetbrains.kotlin.ir.IrStatement
import org.jetbrains.kotlin.ir.builders.*
import org.jetbrains.kotlin.ir.builders.declarations.*
import org.jetbrains.kotlin.ir.declarations.*
import org.jetbrains.kotlin.ir.expressions.*
import org.jetbrains.kotlin.ir.expressions.impl.*
import org.jetbrains.kotlin.ir.symbols.IrSimpleFunctionSymbol
import org.jetbrains.kotlin.ir.types.*
import org.jetbrains.kotlin.ir.util.*
import org.jetbrains.kotlin.ir.visitors.IrElementTransformer
import org.jetbrains.kotlin.name.Name
import org.jetbrains.kotlin.utils.addToStdlib.shouldNotBeCalled

class UpgradeCallableReferences(val context: BackendContext) : FileLoweringPass {
    override fun lower(irFile: IrFile) {
        irFile.transform(UpgradeReflectionReferencesVisitor(context), irFile)
    }
    fun lower(irFunction: IrFunction) {
        irFunction.transform(UpgradeReflectionReferencesVisitor(context), irFunction)
    }

    companion object {
        fun selectSAMOverriddenFunction(type: IrType): IrSimpleFunctionSymbol {
            return type.classOrFail.functions
                .singleOrNull { it.owner.modality == Modality.ABSTRACT }
                ?: error("${type.render()} should have a single abstract method to be a type of function reference")
        }
    }
}


private class UpgradeReflectionReferencesVisitor(val context: BackendContext) : IrElementTransformer<IrDeclarationParent> {

    override fun visitFunctionExpression(expression: IrFunctionExpression, data: IrDeclarationParent): IrElement {
        expression.transformChildren(this, data)
        expression.function.apply {
            require(contextReceiverParametersCount == 0)
            require(dispatchReceiverParameter == null)
            extensionReceiverParameter?.let {
                valueParameters = listOf(it) + valueParameters
                extensionReceiverParameter = null
            }
        }
        return IrBoundFunctionReferenceImpl(
            startOffset = expression.startOffset,
            endOffset = expression.endOffset,
            type = expression.type,
            reflectionTargetSymbol = null,
            overriddenFunctionSymbol = UpgradeCallableReferences.selectSAMOverriddenFunction(expression.type),
            invokeFunction = expression.function,
            origin = expression.origin,
        ).copyAttributes(expression)
    }

    override fun visitFile(declaration: IrFile, data: IrDeclarationParent): IrFile {
        return super.visitFile(declaration, data)
    }

    override fun visitElement(element: IrElement, data: IrDeclarationParent): IrElement {
        if (element is IrDeclarationParent) {
            element.transformChildren(this, element)
        } else {
            element.transformChildren(this, data)
        }
        return element
    }

    override fun visitBlock(expression: IrBlock, data: IrDeclarationParent): IrExpression {
        val origin = expression.origin
        if (
            origin != IrStatementOrigin.ADAPTED_FUNCTION_REFERENCE &&
            origin != IrStatementOrigin.SUSPEND_CONVERSION &&
            origin != IrStatementOrigin.LAMBDA &&
            origin != IrStatementOrigin.FUN_INTERFACE_CONSTRUCTOR_REFERENCE
        ) {
            return super.visitBlock(expression, data)
        }
        require(expression.statements.size == 2)
        val function = expression.statements[0] as IrSimpleFunction
        val (reference, expressionType) = when (val ref = expression.statements[1]) {
            is IrTypeOperatorCall -> {
                require(ref.operator == IrTypeOperator.SAM_CONVERSION)
                ref.argument as IrFunctionReference to ref.typeOperand
            }
            is IrFunctionReference -> ref to ref.type
            else -> shouldNotBeCalled()
        }
        function.transformChildren(this, data)
        reference.transformChildren(this, data)
        require(function.contextReceiverParametersCount == 0)
        require(function.dispatchReceiverParameter == null)
        function.extensionReceiverParameter?.let {
            function.valueParameters = listOf(it) + function.valueParameters
            function.extensionReceiverParameter = null
        }
        return IrBoundFunctionReferenceImpl(
            startOffset = expression.startOffset,
            endOffset = expression.endOffset,
            type = expressionType,
            reflectionTargetSymbol = reference.reflectionTarget.takeUnless { origin.isLambda },
            overriddenFunctionSymbol = UpgradeCallableReferences.selectSAMOverriddenFunction(expressionType),
            invokeFunction = function,
            origin = origin
        ).apply {
            copyAttributes(expression)
            reference.extensionReceiver?.let {
                boundValues.add(it)
            }
        }
    }

    override fun visitDeclaration(declaration: IrDeclarationBase, data: IrDeclarationParent): IrStatement {
        return visitElement(declaration, data) as IrStatement
    }

    override fun visitTypeOperator(expression: IrTypeOperatorCall, data: IrDeclarationParent): IrExpression {
        if (expression.operator == IrTypeOperator.SAM_CONVERSION) {
            expression.transformChildren(this, data)
            val argument = expression.argument
            return if (argument is IrBoundFunctionReference) {
                argument.apply {
                    type = expression.typeOperand
                    overriddenFunctionSymbol = UpgradeCallableReferences.selectSAMOverriddenFunction(expression.typeOperand)
                }
            } else {
                expression
            }
        }
        return super.visitTypeOperator(expression, data)
    }

    override fun visitFunctionReference(expression: IrFunctionReference, data: IrDeclarationParent): IrExpression {
        expression.transformChildren(this, data)
        val arguments = expression.getArgumentsWithIr()
        return IrBoundFunctionReferenceImpl(
            startOffset = expression.startOffset,
            endOffset = expression.endOffset,
            type = expression.type,
            reflectionTargetSymbol = (expression.reflectionTarget ?: expression.symbol).takeUnless { expression.origin.isLambda },
            overriddenFunctionSymbol = UpgradeCallableReferences.selectSAMOverriddenFunction(expression.type),
            invokeFunction = expression.wrapFunction(arguments, data, expression.symbol.owner),
            origin = expression.origin,
        ).apply {
            copyAttributes(expression)
            boundValues += arguments.map { it.second }
        }
    }

    override fun visitPropertyReference(expression: IrPropertyReference, data: IrDeclarationParent): IrExpression {
        expression.transformChildren(this, data)
        val getter = expression.getter?.owner
        val arguments = expression.getArgumentsWithIr()
        return if (getter != null) {
            IrBoundPropertyReferenceImpl(
                startOffset = expression.startOffset,
                endOffset = expression.endOffset,
                type = expression.type,
                reflectionTargetSymbol = expression.symbol,
                getterFunction = expression.wrapFunction(arguments, data, getter),
                setterFunction = expression.setter?.let { expression.wrapFunction(arguments, data, it.owner, isPropertySetter = true) },
                origin = expression.origin,
            )
        } else {
            val field = expression.field!!.owner
            IrBoundPropertyReferenceImpl(
                startOffset = expression.startOffset,
                endOffset = expression.endOffset,
                type = expression.type,
                reflectionTargetSymbol = expression.symbol,
                getterFunction = expression.wrapField(arguments, data, field, isSetter = false),
                setterFunction = if (expression.type.isKMutableProperty()) expression.wrapField(arguments, data, field, isSetter = true) else null,
                origin = expression.origin
            )
        }.apply {
            copyAttributes(expression)
            boundValues += arguments.map { it.second }
        }
    }

    override fun visitLocalDelegatedPropertyReference(expression: IrLocalDelegatedPropertyReference, data: IrDeclarationParent): IrExpression {
        expression.transformChildren(this, data)
        return IrBoundPropertyReferenceImpl(
            startOffset = expression.startOffset,
            endOffset = expression.endOffset,
            type = expression.type,
            reflectionTargetSymbol = expression.symbol,
            getterFunction = expression.wrapFunction(emptyList(), data, expression.getter.owner),
            setterFunction = expression.setter?.let { expression.wrapFunction(emptyList(), data, it.owner, isPropertySetter = true) },
            origin = expression.origin
        )
    }

    private fun IrCallableReference<*>.buildWrapperFunction(
        captured: List<Pair<IrValueParameter, IrExpression>>,
        parent: IrDeclarationParent,
        name: Name,
        isSuspend: Boolean,
        isPropertySetter: Boolean,
        body: IrBlockBodyBuilder.(List<IrValueParameter>) -> Unit,
    ) : IrSimpleFunction {
        val referenceType = this@buildWrapperFunction.type as IrSimpleType
        val referenceTypeArgs = referenceType.arguments.map { it.typeOrNull ?: context.irBuiltIns.anyNType }
        val unboundArgTypes = if (isPropertySetter) referenceTypeArgs else referenceTypeArgs.dropLast(1)
        val returnType = if (isPropertySetter) context.irBuiltIns.unitType else referenceTypeArgs.last()
        val func = context.irFactory.buildFun {
            setSourceRange(this@buildWrapperFunction)
            origin = IrDeclarationOrigin.LOCAL_FUNCTION_FOR_LAMBDA
            this.name = name
            visibility = DescriptorVisibilities.LOCAL
            this.returnType = returnType
            this.isSuspend = isSuspend
        }.apply {
            this.parent = parent
            for (arg in captured) {
                addValueParameter {
                    this.name = arg.first.name
                    this.type = arg.second.type
                }
            }
            var index = 0
            for (type in unboundArgTypes) {
                addValueParameter {
                    this.name = Name.identifier("p${index++}")
                    this.type = type
                }
            }
            this.body = context.createIrBuilder(symbol).run {
                irBlockBody {
                    body(valueParameters)
                }
            }
        }
        return func
    }

    private fun IrPropertyReference.wrapField(
        captured: List<Pair<IrValueParameter, IrExpression>>,
        parent: IrDeclarationParent,
        field: IrField,
        isSetter: Boolean
    ): IrSimpleFunction {
        return buildWrapperFunction(
            captured,
            parent,
            name = Name.special("<${if (isSetter) "set-" else "get-"}${symbol.owner.name}>"),
            isSuspend = false,
            isPropertySetter = isSetter
        ) { params ->
            var index = 0
            val fieldReceiver = when {
                field.isStatic -> null
                else -> irGet(params[index++])
            }
            val exprToReturn = if (isSetter) {
                irSetField(fieldReceiver, field, irGet(params[index++]))
            } else {
                irGetField(fieldReceiver, field)
            }
            require(index == params.size)
            +irReturn(exprToReturn)
        }
    }

    private fun IrCallableReference<*>.wrapFunction(
        captured: List<Pair<IrValueParameter, IrExpression>>,
        parent: IrDeclarationParent,
        referencedFunction: IrFunction,
        isPropertySetter: Boolean = false
    ): IrSimpleFunction {
        return buildWrapperFunction(
            captured,
            parent,
            referencedFunction.name,
            referencedFunction.isSuspend,
            isPropertySetter
        ) { parameters ->
            val exprToReturn = irCallWithSubstitutedType(referencedFunction.symbol, typeArguments = (0 until typeArgumentsCount).map { getTypeArgument(it)!! }).apply {
                val bound = captured.map { it.first}.toSet()
                val (boundParameters, unboundParameters) = referencedFunction.explicitParameters.partition { it in bound}
                require(boundParameters.size + unboundParameters.size == parameters.size) {
                    """
                    |
                    |Parameters of wrapper:
                    |  ${parameters.joinToString("\n  ") { it.render() }}
                    |Bound parameters of function:
                    |  ${boundParameters.joinToString("\n  ") { it.render() }}
                    |Unbound parameters of function:
                    |  ${unboundParameters.joinToString("\n  ") { it.render() }}
                    """.trimMargin()
                }
                for ((parameter, localParameter) in (boundParameters + unboundParameters).zip(parameters)) {
                    putArgument(referencedFunction, parameter, irGet(localParameter))
                }
            }
            +irReturn(exprToReturn)
        }
    }
}