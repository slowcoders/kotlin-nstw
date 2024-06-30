/*
 * Copyright 2010-2024 JetBrains s.r.o. and Kotlin Programming Language contributors.
 * Use of this source code is governed by the Apache 2.0 license that can be found in the license/LICENSE.txt file.
 */

// This file was generated automatically. See compiler/ir/ir.tree/tree-generator/ReadMe.md.
// DO NOT MODIFY IT MANUALLY.

package org.jetbrains.kotlin.ir.expressions

import org.jetbrains.kotlin.ir.declarations.IrSimpleFunction
import org.jetbrains.kotlin.ir.symbols.IrFunctionSymbol
import org.jetbrains.kotlin.ir.symbols.IrSimpleFunctionSymbol
import org.jetbrains.kotlin.ir.util.transformInPlace
import org.jetbrains.kotlin.ir.visitors.IrElementTransformer
import org.jetbrains.kotlin.ir.visitors.IrElementVisitor

/**
 * Generated from: [org.jetbrains.kotlin.ir.generator.IrTree.boundFunctionReference]
 */
abstract class IrBoundFunctionReference : IrExpression() {
    abstract var reflectionTargetSymbol: IrFunctionSymbol?

    abstract var overriddenFunctionSymbol: IrSimpleFunctionSymbol

    abstract val boundValues: MutableList<IrExpression>

    abstract var invokeFunction: IrSimpleFunction

    abstract var origin: IrStatementOrigin?

    override fun <R, D> accept(visitor: IrElementVisitor<R, D>, data: D): R =
        visitor.visitBoundFunctionReference(this, data)

    override fun <D> acceptChildren(visitor: IrElementVisitor<Unit, D>, data: D) {
        boundValues.forEach { it.accept(visitor, data) }
        invokeFunction.accept(visitor, data)
    }

    override fun <D> transformChildren(transformer: IrElementTransformer<D>, data: D) {
        boundValues.transformInPlace(transformer, data)
        invokeFunction = invokeFunction.transform(transformer, data) as IrSimpleFunction
    }
}
