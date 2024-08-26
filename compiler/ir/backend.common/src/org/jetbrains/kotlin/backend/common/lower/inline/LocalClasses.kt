/*
 * Copyright 2010-2024 JetBrains s.r.o. and Kotlin Programming Language contributors.
 * Use of this source code is governed by the Apache 2.0 license that can be found in the license/LICENSE.txt file.
 */

package org.jetbrains.kotlin.backend.common.lower.inline

import org.jetbrains.kotlin.backend.common.*
import org.jetbrains.kotlin.backend.common.lower.LocalClassPopupLowering
import org.jetbrains.kotlin.backend.common.lower.LocalDeclarationsLowering
import org.jetbrains.kotlin.backend.common.lower.locationForExtraction
import org.jetbrains.kotlin.backend.common.lower.moveTo
import org.jetbrains.kotlin.descriptors.DescriptorVisibilities
import org.jetbrains.kotlin.ir.IrElement
import org.jetbrains.kotlin.ir.IrStatement
import org.jetbrains.kotlin.ir.declarations.*
import org.jetbrains.kotlin.ir.expressions.*
import org.jetbrains.kotlin.ir.expressions.impl.IrBlockImpl
import org.jetbrains.kotlin.ir.expressions.impl.IrCompositeImpl
import org.jetbrains.kotlin.ir.irAttribute
import org.jetbrains.kotlin.ir.irFlag
import org.jetbrains.kotlin.ir.util.isAdaptedFunctionReference
import org.jetbrains.kotlin.ir.util.isInlineParameter
import org.jetbrains.kotlin.ir.util.setDeclarationsParent
import org.jetbrains.kotlin.ir.util.*
import org.jetbrains.kotlin.ir.visitors.*
import org.jetbrains.kotlin.utils.addToStdlib.getOrSetIfNull

/*
 Here we're extracting some local classes from inline bodies.
 The mental model of inlining is as following:
  - for inline lambdas, since we don't see the keyword `inline` at a callsite,
    it is logical to think that the lambda won't be copied but will be embedded as is at the callsite,
    so all local classes declared in those inline lambdas are NEVER COPIED.
  - as for the bodies of inline functions, then it is the opposite - we see the `inline` keyword,
    so it is only logical to think that this is a macro substitution, so the bodies of inline functions
    are copied. But the compiler could optimize the usage of some local classes and not copy them.
    So in this case all local classes MIGHT BE COPIED.
 */

class LocalClassesInInlineLambdasLowering(val context: CommonBackendContext) : BodyLoweringPass {
    override fun lower(irFile: IrFile) {
        runOnFilePostfix(irFile)
    }

    override fun lower(irBody: IrBody, container: IrDeclaration) {
        irBody.transformChildren(object : IrElementTransformer<IrDeclarationParent> {
            override fun visitDeclaration(declaration: IrDeclarationBase, data: IrDeclarationParent) =
                super.visitDeclaration(declaration, (declaration as? IrDeclarationParent) ?: data)

            override fun visitCall(expression: IrCall, data: IrDeclarationParent): IrElement {
                val rootCallee = expression.symbol.owner
                if (!rootCallee.isInline)
                    return super.visitCall(expression, data)

                expression.extensionReceiver = expression.extensionReceiver?.transform(this, data)
                expression.dispatchReceiver = expression.dispatchReceiver?.transform(this, data)
                val inlineLambdas = mutableListOf<IrFunction>()
                for (index in 0 until expression.valueArgumentsCount) {
                    val argument = expression.getValueArgument(index)
                    val inlineLambda = (argument as? IrFunctionExpression)?.function
                        ?.takeIf { rootCallee.valueParameters[index].isInlineParameter() }
                    if (inlineLambda == null)
                        expression.putValueArgument(index, argument?.transform(this, data))
                    else
                        inlineLambdas.add(inlineLambda)
                }

                val localClasses = mutableSetOf<IrClass>()
                val localFunctions = mutableSetOf<IrFunction>()
                val adaptedFunctions = mutableSetOf<IrSimpleFunction>()
                val transformer = this
                for (lambda in inlineLambdas) {
                    lambda.acceptChildrenVoid(object : IrElementVisitorVoid {
                        override fun visitElement(element: IrElement) {
                            element.acceptChildrenVoid(this)
                        }

                        override fun visitClass(declaration: IrClass) {
                            declaration.transformChildren(transformer, declaration)

                            localClasses.add(declaration)
                        }

                        override fun visitFunctionExpression(expression: IrFunctionExpression) {
                            expression.function.acceptChildrenVoid(this)
                        }

                        override fun visitFunction(declaration: IrFunction) {
                            declaration.transformChildren(transformer, declaration)

                            localFunctions.add(declaration)
                        }

                        override fun visitCall(expression: IrCall) {
                            val callee = expression.symbol.owner
                            if (!callee.isInline) {
                                expression.acceptChildrenVoid(this)
                                return
                            }

                            expression.extensionReceiver?.acceptVoid(this)
                            expression.dispatchReceiver?.acceptVoid(this)
                            (0 until expression.valueArgumentsCount).forEach { index ->
                                val argument = expression.getValueArgument(index)
                                val parameter = callee.valueParameters[index]
                                // Skip adapted function references - they will be inlined later.
                                if (parameter.isInlineParameter() && argument?.isAdaptedFunctionReference() == true)
                                    adaptedFunctions += (argument as IrBlock).statements[0] as IrSimpleFunction
                                else
                                    argument?.acceptVoid(this)
                            }
                        }
                    })
                }

                if (localClasses.isEmpty() && localFunctions.isEmpty())
                    return expression

                val irBlock = IrBlockImpl(expression.startOffset, expression.endOffset, expression.type).apply {
                    statements += expression
                }
                LocalDeclarationsLowering(context).lower(irBlock, container, data, localClasses, adaptedFunctions)
                irBlock.statements.addAll(0, localClasses)

                for (lambda in inlineLambdas) {
                    lambda.transformChildrenVoid(object : IrElementTransformerVoid() {
                        override fun visitClass(declaration: IrClass): IrStatement {
                            return IrCompositeImpl(
                                declaration.startOffset, declaration.endOffset,
                                context.irBuiltIns.unitType
                            )
                        }
                    })
                }
                localClasses.forEach { it.setDeclarationsParent(data) }

                return irBlock
            }
        }, container as? IrDeclarationParent ?: container.parent)
    }
}

/**
 * A cache of local classes declared in `this` that will need to be extracted later.
 */
private var IrFunction.localClassesToExtract: Set<IrClass>? by irAttribute(followAttributeOwner = false)

fun IrFunction.extractableLocalClasses(): Set<IrClass> {
    localClassesToExtract?.let { return it }

    if (!isInline) return emptySet()
    // Conservatively assume that functions with reified type parameters must be copied.
    if (typeParameters.any { it.isReified }) return emptySet()

    val crossinlineParameters = valueParameters.filter { it.isCrossinline }.toSet()

    return buildSet {
        acceptChildrenVoid(object : IrElementVisitorVoid {
            override fun visitElement(element: IrElement) {
                element.acceptChildrenVoid(this)
            }

            override fun visitClass(declaration: IrClass) {
                var canExtract = true
                if (crossinlineParameters.isNotEmpty()) {
                    declaration.acceptVoid(object : IrElementVisitorVoid {
                        override fun visitElement(element: IrElement) {
                            element.acceptChildrenVoid(this)
                        }

                        override fun visitGetValue(expression: IrGetValue) {
                            if (expression.symbol.owner in crossinlineParameters)
                                canExtract = false
                        }
                    })
                }
                if (canExtract)
                    add(declaration)
            }
        })
    }.also {
        localClassesToExtract = it
    }
}

/**
 * Remaps symbols in local classes inside `this` to their corresponding lifted classes.
 */
var IrFunction.localClassSymbolRemapper: DeepCopySymbolRemapper? by irAttribute(followAttributeOwner = true)

/**
 * Used to mark local classes for which we lifted their copies.
 * These local classes should be eliminated during inlining.
 */
var IrClass.useExtractedCopy: Boolean by irFlag(followAttributeOwner = true)

/**
 * Runs [LocalClassPopupLowering] so that extractable local classes are lowered to the point when they no longer capture any local context,
 * and then lift copies of those local classes out of the inline function so that those lifted copies could be reused across
 * call sites in the same module as the inline function.
 *
 * The inline function itself still uses the original local classes, not their copies.
 * The usages of copies are inserted to call sites during inlining.
 */
class LocalClassesInInlineFunctionsLowering(val context: CommonBackendContext) : BodyLoweringPass {
    override fun lower(irFile: IrFile) {
        runOnFilePostfix(irFile)
    }

    override fun lower(irBody: IrBody, container: IrDeclaration) {
        val function = container as? IrFunction ?: return
        val classesToExtract = function.extractableLocalClasses()
        if (classesToExtract.isEmpty())
            return

        LocalDeclarationsLowering(context).lower(function, function, classesToExtract)

        // TODO: Test a case when one local class references another local class
        val symbolRemapper = function::localClassSymbolRemapper.getOrSetIfNull(::DeepCopySymbolRemapper)
        for (klass in classesToExtract) {
            val locationForExtraction = klass.locationForExtraction()

            // Create new symbols and keep them in `function.localClassSymbolRemapper`, so that later during
            // inlining we could use that same remapper to replace usages of local classes with usages of lifted classes.
            klass.acceptVoid(symbolRemapper)
            val classCopy = klass.transform(DeepCopyIrTreeWithSymbols(symbolRemapper), null) as IrClass

            // Extracted local classes will only be shared across call sites in the same module,
            classCopy.visibility = DescriptorVisibilities.INTERNAL

            classCopy.moveTo(locationForExtraction)
            classCopy.patchDeclarationParents()
            klass.useExtractedCopy = true
        }
    }
}
