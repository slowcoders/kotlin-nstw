/*
 * Copyright 2010-2023 JetBrains s.r.o. and Kotlin Programming Language contributors.
 * Use of this source code is governed by the Apache 2.0 license that can be found in the license/LICENSE.txt file.
 */

package org.jetbrains.kotlin.analysis.api.fir.types

import org.jetbrains.kotlin.analysis.api.KaAnalysisNonPublicApi
import org.jetbrains.kotlin.analysis.api.KaSession
import org.jetbrains.kotlin.analysis.api.annotations.KaAnnotationsList
import org.jetbrains.kotlin.analysis.api.fir.KaFirSession
import org.jetbrains.kotlin.analysis.api.fir.KaSymbolByFirBuilder
import org.jetbrains.kotlin.analysis.api.fir.annotations.KaFirAnnotationListForType
import org.jetbrains.kotlin.analysis.api.fir.getCandidateSymbols
import org.jetbrains.kotlin.analysis.api.fir.types.qualifiers.ErrorClassTypeQualifierBuilder
import org.jetbrains.kotlin.analysis.api.fir.utils.ConeClassLikeTypePointer
import org.jetbrains.kotlin.analysis.api.fir.utils.ConeDiagnosticPointer
import org.jetbrains.kotlin.analysis.api.fir.utils.ConeErrorTypePointer
import org.jetbrains.kotlin.analysis.api.fir.utils.ConeTypePointer
import org.jetbrains.kotlin.analysis.api.fir.utils.buildAbbreviatedType
import org.jetbrains.kotlin.analysis.api.fir.utils.cached
import org.jetbrains.kotlin.analysis.api.lifetime.KaLifetimeToken
import org.jetbrains.kotlin.analysis.api.lifetime.withValidityAssertion
import org.jetbrains.kotlin.analysis.api.symbols.KaClassLikeSymbol
import org.jetbrains.kotlin.analysis.api.types.KaClassErrorType
import org.jetbrains.kotlin.analysis.api.types.KaClassTypeQualifier
import org.jetbrains.kotlin.analysis.api.types.KaTypeNullability
import org.jetbrains.kotlin.analysis.api.types.KaTypePointer
import org.jetbrains.kotlin.analysis.api.types.KaUsualClassType
import org.jetbrains.kotlin.analysis.utils.errors.requireIsInstance
import org.jetbrains.kotlin.fir.diagnostics.ConeDiagnostic
import org.jetbrains.kotlin.fir.resolve.diagnostics.ConeUnmatchedTypeArgumentsError
import org.jetbrains.kotlin.fir.resolve.diagnostics.ConeUnresolvedError
import org.jetbrains.kotlin.fir.resolve.diagnostics.ConeUnresolvedSymbolError
import org.jetbrains.kotlin.fir.symbols.impl.FirClassLikeSymbol
import org.jetbrains.kotlin.fir.types.ConeClassLikeType
import org.jetbrains.kotlin.fir.types.ConeErrorType
import org.jetbrains.kotlin.fir.types.ConeNullability
import org.jetbrains.kotlin.fir.types.renderForDebugging

internal class KaFirClassErrorType(
    override val coneType: ConeClassLikeType,
    private val coneNullability: ConeNullability,
    private val coneDiagnostic: ConeDiagnostic,
    private val builder: KaSymbolByFirBuilder,
) : KaClassErrorType(), KaFirType {
    override val token: KaLifetimeToken get() = builder.token

    override val qualifiers: List<KaClassTypeQualifier> by cached {
        when (coneDiagnostic) {
            is ConeUnresolvedError ->
                ErrorClassTypeQualifierBuilder.createQualifiersForUnresolvedType(coneDiagnostic, builder)
            is ConeUnmatchedTypeArgumentsError ->
                ErrorClassTypeQualifierBuilder.createQualifiersForUnmatchedTypeArgumentsType(coneDiagnostic, builder)
            else -> error("Unsupported ${coneDiagnostic::class}")
        }
    }

    override val nullability: KaTypeNullability get() = withValidityAssertion { coneType.nullability.asKtNullability() }

    @KaAnalysisNonPublicApi
    override val presentableText: String?
        get() = withValidityAssertion {
            qualifiers.joinToString(separator = ".") { it.name.asString() }
        }

    @KaAnalysisNonPublicApi
    override val errorMessage: String get() = withValidityAssertion { coneDiagnostic.reason }

    override val annotationsList: KaAnnotationsList by cached {
        KaFirAnnotationListForType.create(coneType, builder)
    }

    override val candidateSymbols: Collection<KaClassLikeSymbol> by cached {
        val symbols = coneDiagnostic.getCandidateSymbols().filterIsInstance<FirClassLikeSymbol<*>>()
        symbols.map { builder.classifierBuilder.buildClassLikeSymbol(it) }
    }

    override val abbreviatedType: KaUsualClassType? by cached {
        builder.buildAbbreviatedType(coneType)
    }

    override fun equals(other: Any?) = typeEquals(other)
    override fun hashCode() = typeHashcode()
    override fun toString() = coneType.renderForDebugging()

    @KaAnalysisNonPublicApi
    override fun createPointer(): KaTypePointer<KaClassErrorType> = withValidityAssertion {
        val coneTypePointer = if (coneType !is ConeErrorType) {
            val classSymbol = builder.classifierBuilder.buildClassLikeSymbolByLookupTag(coneType.lookupTag)
            if (classSymbol != null) {
                ConeClassLikeTypePointer(classSymbol, coneType, builder)
            } else {
                val coneErrorType = ConeErrorType(
                    ConeUnresolvedSymbolError(coneType.lookupTag.classId),
                    isUninferredParameter = false,
                    delegatedType = null,
                    typeArguments = coneType.typeArguments,
                    attributes = coneType.attributes
                )
                ConeErrorTypePointer(coneErrorType, builder)
            }
        } else {
            ConeErrorTypePointer(coneType, builder)
        }

        val coneDiagnosticPointer = ConeDiagnosticPointer.create(coneDiagnostic, builder)

        KaFirClassErrorTypePointer(coneTypePointer, coneNullability, coneDiagnosticPointer)
    }
}

@KaAnalysisNonPublicApi
private class KaFirClassErrorTypePointer(
    private val typePointer: ConeTypePointer,
    private val nullability: ConeNullability,
    private val diagnosticPointer: ConeDiagnosticPointer
) : KaTypePointer<KaClassErrorType> {
    override fun restore(session: KaSession): KaClassErrorType? = session.withValidityAssertion {
        requireIsInstance<KaFirSession>(session)

        val coneType = typePointer.restore(session) as? ConeClassLikeType ?: return null
        val coneDiagnostic = diagnosticPointer.restore(session) ?: return null

        return KaFirClassErrorType(coneType, nullability, coneDiagnostic, session.firSymbolBuilder)
    }
}