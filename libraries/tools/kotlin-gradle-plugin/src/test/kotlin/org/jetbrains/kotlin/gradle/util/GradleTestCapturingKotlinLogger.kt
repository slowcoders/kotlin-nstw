/*
 * Copyright 2010-2025 JetBrains s.r.o. and Kotlin Programming Language contributors.
 * Use of this source code is governed by the Apache 2.0 license that can be found in the license/LICENSE.txt file.
 */

package org.jetbrains.kotlin.gradle.util

import org.jetbrains.kotlin.buildtools.api.KotlinLogger

class GradleTestCapturingKotlinLogger : KotlinLogger {
    private val _messages = mutableListOf<String>()
    val messages: List<String> get() = _messages

    private val _exceptions = mutableListOf<Throwable>()
    val exceptions: List<Throwable> get() = _exceptions

    override val isDebugEnabled: Boolean
        get() = true

    override fun error(msg: String, throwable: Throwable?) {
        _messages.add(msg)
        _exceptions.add(throwable ?: return)
    }

    override fun warn(msg: String, throwable: Throwable?) {
        _messages.add(msg)
        _exceptions.add(throwable ?: return)
    }

    override fun info(msg: String) {
        _messages.add(msg)
    }

    override fun debug(msg: String) {
        _messages.add(msg)
    }

    override fun lifecycle(msg: String) {
        _messages.add(msg)
    }
}