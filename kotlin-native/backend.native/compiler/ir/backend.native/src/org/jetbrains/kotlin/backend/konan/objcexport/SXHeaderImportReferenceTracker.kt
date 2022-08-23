/*
 * Copyright 2010-2022 JetBrains s.r.o. and Kotlin Programming Language contributors.
 * Use of this source code is governed by the Apache 2.0 license that can be found in the license/LICENSE.txt file.
 */

package org.jetbrains.kotlin.backend.konan.objcexport

import org.jetbrains.kotlin.backend.konan.descriptors.isInterface
import org.jetbrains.kotlin.backend.konan.objcexport.sx.SXObjCHeader
import org.jetbrains.kotlin.descriptors.ClassDescriptor
import org.jetbrains.kotlin.types.error.ErrorUtils

internal class SXHeaderImportReferenceTracker(
        private val header: SXObjCHeader,
        private val resolver: CrossModuleResolver,
        private val mapper: ObjCExportMapper,
        private val eventQueue: EventQueue,
) : ReferenceTracker {

    private fun getClassOrProtocolName(descriptor: ClassDescriptor): ObjCExportNamer.ClassOrProtocolName {
        assert(mapper.shouldBeExposed(descriptor)) { "Shouldn't be exposed: $descriptor" }
        if (ErrorUtils.isError(descriptor)) {
            return ObjCExportNamer.ClassOrProtocolName("ERROR", "ERROR")
        }

        return resolver.findNamer(descriptor).getClassOrProtocolName(descriptor)
    }

    override fun trackReference(declaration: ClassDescriptor): ObjCExportNamer.ClassOrProtocolName {
        if (declaration.isInterface) {
            eventQueue.add(Event.TranslateInterface(declaration))
        } else {
            eventQueue.add(Event.TranslateClass(declaration))
        }
        val name = getClassOrProtocolName(declaration)
        // Add a forward declaration because we don't control layout of the declarations in the header.
        if (resolver.findModuleBuilder(declaration).findHeaderForDeclaration(declaration) == header) {
            if (declaration.isInterface) {
                eventQueue.add(Event.TranslateInterfaceForwardDeclaration(declaration))
            } else {
                eventQueue.add(Event.TranslateClassForwardDeclaration(declaration))
            }
        } else {
            val declarationHeader = resolver.findModuleBuilder(declaration).findHeaderForDeclaration(declaration)
                    ?: error("$declaration has no header")
            header.addImport(declarationHeader)
        }
        return name
    }

    override fun trackClassForwardDeclaration(forwardDeclaration: ObjCClassForwardDeclaration) {
        header.addClassForwardDeclaration(forwardDeclaration)
    }

    override fun trackProtocolForwardDeclaration(objCName: String) {
        header.addProtocolForwardDeclaration(objCName)
    }
}