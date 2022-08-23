/*
 * Copyright 2010-2022 JetBrains s.r.o. and Kotlin Programming Language contributors.
 * Use of this source code is governed by the Apache 2.0 license that can be found in the license/LICENSE.txt file.
 */

package org.jetbrains.kotlin.backend.konan.objcexport

interface EventQueue {
    fun add(event: Event)
}

interface EventProcessor {

    fun begin()

    fun process(event: Event)

    fun finalize()
}