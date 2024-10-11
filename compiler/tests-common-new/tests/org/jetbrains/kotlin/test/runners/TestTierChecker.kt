/*
 * Copyright 2010-2024 JetBrains s.r.o. and Kotlin Programming Language contributors.
 * Use of this source code is governed by the Apache 2.0 license that can be found in the license/LICENSE.txt file.
 */

package org.jetbrains.kotlin.test.runners

import org.jetbrains.kotlin.platform.isCommon
import org.jetbrains.kotlin.platform.jvm.isJvm
import org.jetbrains.kotlin.test.WrappedException
import org.jetbrains.kotlin.test.directives.CodegenTestDirectives.IGNORE_BACKEND
import org.jetbrains.kotlin.test.directives.CodegenTestDirectives.IGNORE_BACKEND_K2
import org.jetbrains.kotlin.test.directives.CodegenTestDirectives.IGNORE_BACKEND_K2_MULTI_MODULE
import org.jetbrains.kotlin.test.directives.CodegenTestDirectives.IGNORE_BACKEND_MULTI_MODULE
import org.jetbrains.kotlin.test.directives.FirDiagnosticsDirectives.DISABLE_NEXT_TIER_SUGGESTION
import org.jetbrains.kotlin.test.directives.FirDiagnosticsDirectives.RUN_PIPELINE_TILL
import org.jetbrains.kotlin.test.frontend.fir.shouldRunFirFrontendFacade
import org.jetbrains.kotlin.test.model.*
import org.jetbrains.kotlin.test.services.TestModuleStructure
import org.jetbrains.kotlin.test.services.TestServices
import org.jetbrains.kotlin.test.services.assertions
import org.jetbrains.kotlin.test.services.moduleStructure
import org.jetbrains.kotlin.test.utils.*
import org.jetbrains.kotlin.utils.addToStdlib.partitionNotNull
import org.jetbrains.kotlin.utils.joinToEnglishAndString
import org.jetbrains.kotlin.utils.joinToEnglishOrString
import org.opentest4j.AssertionFailedError
import java.io.File
import java.util.*

/**
 * The list of all possible test tier labels one can use in
 * [RUN_PIPELINE_TILL][org.jetbrains.kotlin.test.directives.FirDiagnosticsDirectives.RUN_PIPELINE_TILL].
 * Prefer obtaining [TestTier] though, as some tiers may not be applicable for some tests.
 */
enum class TestTierLabel {
    FRONTEND,
    FIR2IR,
    KLIB,
    BACKEND;

    fun toTier(list: List<TestTier>): TestTier = TestTier(this, list)
}

class TierPassesMarker(val tier: TestTier, val module: TestModule, val origin: AnalysisHandler<*>) : Exception() {
    override val message: String
        get() = "Looks like tier $tier passes with no error diagnostics. Please update the tier directive to `// $RUN_PIPELINE_TILL: ${tier.next}`${
            suggestSubsequentTiersIfAppropriate(tier.next)
        } and regenerate tests."
}

private fun TestTier.allSubsequent(): List<TestTier> = when {
    isLast -> emptyList()
    else -> generateSequence(next) { if (!it.isLast) it.next else null }.toList()
}

private fun suggestSubsequentTiersIfAppropriate(tier: TestTier): String =
    when {
        tier.isLast -> ""
        else -> " (maybe even ${tier.allSubsequent().joinToEnglishOrString()} if you are sure)"
    }

class TestTier(
    val label: TestTierLabel,
    /**
     * Encapsulates the logic of obtaining the [next] tier into the current tier itself.
     */
    private val container: List<TestTier>,
) {
    val name get() = label.name
    override fun toString(): String = name

    val next: TestTier get() = container[container.indexOf(this) + 1]

    val isLast: Boolean get() = container.last() == this
}

fun Collection<TestTierLabel>.toTiers(): List<TestTier> =
    buildList {
        for (constructor in this@toTiers) {
            add(constructor.toTier(this))
        }
    }

/**
 * The tiers this module is expected to be compiled at by its nature.
 * For example, if this is a common module, it should only be compiled up to
 * [TestTierLabel.KLIB], and if this is a JVM module - up to [TestTierLabel.BACKEND] while
 * _not_ including [TestTierLabel.KLIB].
 */
private fun TestModuleStructure.applicableTestTiersFor(module: TestModule): TreeSet<TestTierLabel> {
    // Sometimes we don't run FIR for common modules.
    // See: `compiler/testData/diagnostics/testsWithStdLib/multiplatform/actualExternalInJs.kt`
    if (!shouldRunFirFrontendFacade(module, this, testModulesByName)) {
        return sortedSetOf()
    }

    val tiers = sortedSetOf(TestTierLabel.FRONTEND, TestTierLabel.FIR2IR)

    if (!module.targetPlatform.isJvm()) {
        tiers.add(TestTierLabel.KLIB)
    }

    if (!module.targetPlatform.isCommon()) {
        if (IGNORE_BACKEND_DIRECTIVES.none { directive -> module.targetBackend in module.directives[directive] }) {
            tiers.add(TestTierLabel.BACKEND)
        }
    }

    return tiers
}

val TestModuleStructure.applicableTestTiers: List<TestTier>
    get() = modules
        .map { applicableTestTiersFor(it) }
        .reduce { acc, next -> acc.apply { addAll(next) } }
        .toTiers()

private val IGNORE_BACKEND_DIRECTIVES = listOf(
    IGNORE_BACKEND,
    IGNORE_BACKEND_K2,
    IGNORE_BACKEND_MULTI_MODULE,
    IGNORE_BACKEND_K2_MULTI_MODULE,
)

class TestTierChecker(
    private val lastTierCurrentPipelineExecutes: TestTierLabel,
    private val numberOfMarkerHandlersPerModule: Int,
    testServices: TestServices,
) : AfterAnalysisChecker(testServices) {
    companion object {
        val UPDATE_TIERS_AUTOMATICALLY = System.getProperty("update-tiers-automatically", "true") == "true"
        const val TIERED_FAILURE_EXTENSION = ".tiered-failure.txt"
    }

    override fun check(failedAssertions: List<WrappedException>) {}

    private fun analyzeFailures(successfulTierMarkers: List<TierPassesMarker>, thereAreOtherFailures: Boolean): TieredHandlerException? {
        val applicableTiers = testServices.moduleStructure.applicableTestTiers
        val applicableTierLabels = applicableTiers.map { it.label }
        val disableText = testServices.moduleStructure.allDirectives[DISABLE_NEXT_TIER_SUGGESTION]
        val declaredTier = testServices.moduleStructure.allDirectives[RUN_PIPELINE_TILL].firstOrNull()?.let(TestTierLabel::valueOf)

        if (DISABLE_NEXT_TIER_SUGGESTION in testServices.moduleStructure.allDirectives && disableText.isEmpty()) {
            return "Please specify the reason why the tier upgrade suggestion is disabled in `// $DISABLE_NEXT_TIER_SUGGESTION`"
                .let(::TieredHandlerException)
        }

        if (successfulTierMarkers.any { it.tier.label != lastTierCurrentPipelineExecutes }) {
            return "Markers of tiers other than $lastTierCurrentPipelineExecutes should not be thrown when using a $lastTierCurrentPipelineExecutes runner. The test runner probably contains some redundant early-tier handlers"
                .let(::TieredHandlerException)
        }

        if (declaredTier != null && declaredTier !in applicableTierLabels) {
            setTier(applicableTiers.first().label)
            return "The test declares the tier $declaredTier, but is configured to only run ${applicableTiers.joinToEnglishAndString()}. Please set an appropriate tier and regenerate tests"
                .let(::TieredHandlerException)
        }

        if (lastTierCurrentPipelineExecutes !in applicableTierLabels) {
            throw "This is a $lastTierCurrentPipelineExecutes test runner, it's not applicable for tests configured to run ${applicableTiers.joinToEnglishAndString()}. Please regenerate tests"
                .let(::TieredHandlerException)
        }

        val moduleToSuccessMarkers = successfulTierMarkers
            .groupingBy { it.module }
            .aggregate<TierPassesMarker, TestModule, MutableSet<AnalysisHandler<*>>> { _, acc, element, _ ->
                acc?.apply { add(element.origin) } ?: mutableSetOf(element.origin)
            }

        val everyModuleIsSuccessful = testServices.moduleStructure.modules.all {
            lastTierCurrentPipelineExecutes !in testServices.moduleStructure.applicableTestTiersFor(it)
                    || moduleToSuccessMarkers[it]?.size == numberOfMarkerHandlersPerModule
        }

        if (declaredTier != null && lastTierCurrentPipelineExecutes < declaredTier) {
            return "A $lastTierCurrentPipelineExecutes runner is not enough for a $declaredTier test. Please regenerate tests"
                .let(::TieredHandlerException)
        }

        if (everyModuleIsSuccessful && !thereAreOtherFailures) {
            if (DISABLE_NEXT_TIER_SUGGESTION in testServices.moduleStructure.allDirectives) {
                return null
            }

            setTier(successfulTierMarkers.first().tier.next.label)
            return successfulTierMarkers.first().message
                .let(::TieredHandlerException)
        }

        return null
    }

    override fun suppressIfNeeded(failedAssertions: List<WrappedException>): List<WrappedException> {
        val (successfulTierMarkers, otherFailures) = failedAssertions.partitionNotNull {
            (it as? WrappedException.FromHandler)?.cause as? TierPassesMarker
        }

        if (otherFailures.isNotEmpty()) {
            if (!isDirectiveSetInTest) {
                setTier(testServices.moduleStructure.applicableTestTiers.first().label)
            }
        }

        val ownException = analyzeFailures(successfulTierMarkers, otherFailures.isNotEmpty())
        val originalFile = testServices.moduleStructure.modules.first().files.first().originalFile
        val failFile = File(originalFile.path.removeSuffix(".kt") + TIERED_FAILURE_EXTENSION)

        return otherFailures + when {
            ownException != null -> when {
                !failFile.exists() -> WrappedException.FromAfterAnalysisChecker(ownException).let(::listOf)
                else -> try {
                    testServices.assertions.assertEqualsToFile(failFile, ownException.message, sanitizer = { it.replace("\r", "").trim() })
                    emptyList()
                } catch (a: AssertionFailedError) {
                    WrappedException.FromAfterAnalysisChecker(a).let(::listOf)
                }
            }
            failFile.exists() -> "There's no error from ${this::class.simpleName}, removing the `$TIERED_FAILURE_EXTENSION` file. Please rerun the test"
                .let(::Exception)
                .let(WrappedException::FromAfterAnalysisChecker)
                .let(::listOf)
                .also { failFile.delete() }
            else -> emptyList()
        }
    }

    private fun setTier(label: TestTierLabel) {
        if (!UPDATE_TIERS_AUTOMATICALLY) {
            return
        }

        fun File.writeDirective() {
            if (exists()) {
                if (isDirectiveSetInTest) {
                    writeText(readText().replace("// $RUN_PIPELINE_TILL: .*".toRegex(), "// $RUN_PIPELINE_TILL: $label"))
                } else {
                    writeText("// $RUN_PIPELINE_TILL: $label\n" + readText())
                }
            }
        }

        val originalFile = testServices.moduleStructure.modules.first().files.first().originalFile.originalTestDataFile
        val failFile = File(originalFile.path.removeSuffix(".kt") + TIERED_FAILURE_EXTENSION)

        if (failFile.exists()) {
            return
        }

        listOf(
            originalFile,
            originalFile.firTestDataFile,
            originalFile.llFirTestDataFile,
            originalFile.latestLVTestDataFile,
            originalFile.reversedTestDataFile,
        ).forEach(File::writeDirective)
    }

    private val isDirectiveSetInTest: Boolean
        get() {
            val originalFile = testServices.moduleStructure.modules.first().files.first().originalFile.originalTestDataFile
            return originalFile.readLines().any { it.startsWith("// $RUN_PIPELINE_TILL: ") }
        }

    private class TieredHandlerException(override val message: String) : Exception(message)
}
