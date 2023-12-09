/*
 * Copyright 2010-2023 JetBrains s.r.o. and Kotlin Programming Language contributors.
 * Use of this source code is governed by the Apache 2.0 license that can be found in the license/LICENSE.txt file.
 */

package org.jetbrains.kotlin.bir.backend/*
 * Copyright 2010-2023 JetBrains s.r.o. and Kotlin Programming Language contributors.
 * Use of this source code is governed by the Apache 2.0 license that can be found in the license/LICENSE.txt file.
 */

import org.jetbrains.kotlin.bir.*
import org.jetbrains.kotlin.bir.declarations.BirModuleFragment

/*
 * Copyright 2010-2023 JetBrains s.r.o. and Kotlin Programming Language contributors.
 * Use of this source code is governed by the Apache 2.0 license that can be found in the license/LICENSE.txt file.
 */

context(BirBackendContext)
abstract class BirLoweringPhase {
    abstract operator fun invoke(module: BirModuleFragment)

    protected inline fun <reified E : BirElement> registerIndexKey(
        includeExternalModules: Boolean,
        crossinline condition: (E) -> Boolean,
    ): BirElementsIndexKey<E> =
        registerIndexKey<E>(includeExternalModules, { element -> condition(element as E) }, E::class.java)

    protected inline fun <reified E : BirElement> registerIndexKey(includeExternalModules: Boolean): BirElementsIndexKey<E> =
        registerIndexKey<E>(includeExternalModules, null, E::class.java)

    protected fun <E : BirElement> registerIndexKey(
        includeExternalModules: Boolean,
        condition: BirElementIndexMatcher?,
        elementClass: Class<*>,
    ): BirElementsIndexKey<E> {
        val key = BirElementsIndexKey<E>(condition, elementClass)
        compiledBir.registerElementIndexingKey(key)
        if (includeExternalModules) {
            externalModulesBir.registerElementIndexingKey(key)
        }

        return key
    }

    protected fun <E : BirElement, R : BirElement> registerBackReferencesKey(
        block: BirElementBackReferenceRecorder<R>,
        elementClass: Class<*>,
    ): BirElementBackReferencesKey<E, R> {
        val key = BirElementBackReferencesKey<E, R>(block, elementClass)
        compiledBir.registerElementBackReferencesKey(key)
        return key
    }

    protected inline fun <reified E : BirElement, R : BirElement> registerMultipleBackReferencesKey(
        crossinline block: context(BirElementBackReferenceRecorderScope) (E) -> Unit,
    ): BirElementBackReferencesKey<E, R> = registerBackReferencesKey<E, R>(object : BirElementBackReferenceRecorder<R> {
        context(BirElementBackReferenceRecorderScope)
        override fun recordBackReferences(element: BirElementBase) {
            if (element is E) {
                block(this@BirElementBackReferenceRecorderScope, element)
            }
        }
    }, E::class.java)

    protected inline fun <reified E : BirElement, R : BirElement> registerBackReferencesKey(
        crossinline block: (E) -> R?,
    ): BirElementBackReferencesKey<E, R> = registerBackReferencesKey<E, R>(object : BirElementBackReferenceRecorder<R> {
        context(BirElementBackReferenceRecorderScope)
        override fun recordBackReferences(element: BirElementBase) {
            if (element is E) {
                recordReference(block(element))
            }
        }
    }, E::class.java)

    protected fun <E : BirElement> getAllElementsWithIndex(key: BirElementsIndexKey<E>): Sequence<E> {
        var elements = compiledBir.getElementsWithIndex(key)
        if (externalModulesBir.hasIndex(key)) {
            elements += externalModulesBir.getElementsWithIndex(key)
        }
        return elements
    }


    protected fun <E : BirElement, T> acquireProperty(property: BirElementDynamicPropertyKey<E, T>): BirElementDynamicPropertyToken<E, T> {
        return dynamicPropertyManager.acquireProperty(property)
    }

    protected inline fun <reified E : BirElement, T> acquireTemporaryProperty(): BirElementDynamicPropertyToken<E, T> {
        val property = BirElementDynamicPropertyKey<E, T>()
        return acquireProperty(property)
    }


    protected fun <T> lz(initializer: () -> T) = lazy(LazyThreadSafetyMode.NONE, initializer)
}