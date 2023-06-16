/*
 * Copyright 2010-2023 JetBrains s.r.o. and Kotlin Programming Language contributors.
 * Use of this source code is governed by the Apache 2.0 license that can be found in the license/LICENSE.txt file.
 */

package org.jetbrains.kotlin.test.runners;

import com.intellij.testFramework.TestDataPath;
import org.jetbrains.kotlin.test.util.KtTestUtil;
import org.jetbrains.kotlin.test.TargetBackend;
import org.jetbrains.kotlin.test.TestMetadata;
import org.junit.jupiter.api.Nested;
import org.junit.jupiter.api.Test;

import java.io.File;
import java.util.regex.Pattern;

/** This class is generated by {@link org.jetbrains.kotlin.test.generators.GenerateCompilerTestsKt}. DO NOT MODIFY MANUALLY */
@SuppressWarnings("all")
@TestMetadata("compiler/testData/diagnostics/tests/multiplatform")
@TestDataPath("$PROJECT_ROOT")
public class FirOldFrontendMPPDiagnosticsWithLightTreeTestGenerated extends AbstractFirLightTreeWithActualizerDiagnosticsTest {
    @Test
    public void testAllFilesPresentInMultiplatform() throws Exception {
        KtTestUtil.assertAllTestsPresentByMetadataWithExcluded(this.getClass(), new File("compiler/testData/diagnostics/tests/multiplatform"), Pattern.compile("^(.*)\\.kts?$"), Pattern.compile("^(.+)\\.(reversed|fir|ll)\\.kts?$"), TargetBackend.JVM_IR, true);
    }

    @Test
    @TestMetadata("expectAbstractToString.kt")
    public void testExpectAbstractToString() throws Exception {
        runTest("compiler/testData/diagnostics/tests/multiplatform/expectAbstractToString.kt");
    }

    @Test
    @TestMetadata("expectDataObject.kt")
    public void testExpectDataObject() throws Exception {
        runTest("compiler/testData/diagnostics/tests/multiplatform/expectDataObject.kt");
    }

    @Test
    @TestMetadata("expectExternal.kt")
    public void testExpectExternal() throws Exception {
        runTest("compiler/testData/diagnostics/tests/multiplatform/expectExternal.kt");
    }

    @Test
    @TestMetadata("expectInterfaceApplicability.kt")
    public void testExpectInterfaceApplicability() throws Exception {
        runTest("compiler/testData/diagnostics/tests/multiplatform/expectInterfaceApplicability.kt");
    }

    @Test
    @TestMetadata("expectObjectWithAbstractMember.kt")
    public void testExpectObjectWithAbstractMember() throws Exception {
        runTest("compiler/testData/diagnostics/tests/multiplatform/expectObjectWithAbstractMember.kt");
    }

    @Test
    @TestMetadata("expectTailrec.kt")
    public void testExpectTailrec() throws Exception {
        runTest("compiler/testData/diagnostics/tests/multiplatform/expectTailrec.kt");
    }

    @Test
    @TestMetadata("headerFunInNonHeaderClass.kt")
    public void testHeaderFunInNonHeaderClass() throws Exception {
        runTest("compiler/testData/diagnostics/tests/multiplatform/headerFunInNonHeaderClass.kt");
    }

    @Test
    @TestMetadata("implDelegatedMember.kt")
    public void testImplDelegatedMember() throws Exception {
        runTest("compiler/testData/diagnostics/tests/multiplatform/implDelegatedMember.kt");
    }

    @Test
    @TestMetadata("implDynamic.kt")
    public void testImplDynamic() throws Exception {
        runTest("compiler/testData/diagnostics/tests/multiplatform/implDynamic.kt");
    }

    @Test
    @TestMetadata("implFakeOverride.kt")
    public void testImplFakeOverride() throws Exception {
        runTest("compiler/testData/diagnostics/tests/multiplatform/implFakeOverride.kt");
    }

    @Test
    @TestMetadata("incDecOperatorsInExpectClass.kt")
    public void testIncDecOperatorsInExpectClass() throws Exception {
        runTest("compiler/testData/diagnostics/tests/multiplatform/incDecOperatorsInExpectClass.kt");
    }

    @Test
    @TestMetadata("incompatibles.kt")
    public void testIncompatibles() throws Exception {
        runTest("compiler/testData/diagnostics/tests/multiplatform/incompatibles.kt");
    }

    @Test
    @TestMetadata("kt54827.kt")
    public void testKt54827() throws Exception {
        runTest("compiler/testData/diagnostics/tests/multiplatform/kt54827.kt");
    }

    @Test
    @TestMetadata("kt58153.kt")
    public void testKt58153() throws Exception {
        runTest("compiler/testData/diagnostics/tests/multiplatform/kt58153.kt");
    }

    @Test
    @TestMetadata("manyImplMemberNotImplemented.kt")
    public void testManyImplMemberNotImplemented() throws Exception {
        runTest("compiler/testData/diagnostics/tests/multiplatform/manyImplMemberNotImplemented.kt");
    }

    @Test
    @TestMetadata("manyInterfacesMemberNotImplemented.kt")
    public void testManyInterfacesMemberNotImplemented() throws Exception {
        runTest("compiler/testData/diagnostics/tests/multiplatform/manyInterfacesMemberNotImplemented.kt");
    }

    @Test
    @TestMetadata("modifierApplicability.kt")
    public void testModifierApplicability() throws Exception {
        runTest("compiler/testData/diagnostics/tests/multiplatform/modifierApplicability.kt");
    }

    @Test
    @TestMetadata("multipleExpectInterfacesImplementation.kt")
    public void testMultipleExpectInterfacesImplementation() throws Exception {
        runTest("compiler/testData/diagnostics/tests/multiplatform/multipleExpectInterfacesImplementation.kt");
    }

    @Test
    @TestMetadata("namedArguments.kt")
    public void testNamedArguments() throws Exception {
        runTest("compiler/testData/diagnostics/tests/multiplatform/namedArguments.kt");
    }

    @Test
    @TestMetadata("privateTopLevelDeclarations.kt")
    public void testPrivateTopLevelDeclarations() throws Exception {
        runTest("compiler/testData/diagnostics/tests/multiplatform/privateTopLevelDeclarations.kt");
    }

    @Test
    @TestMetadata("sealedClassWithPrivateConstructor.kt")
    public void testSealedClassWithPrivateConstructor() throws Exception {
        runTest("compiler/testData/diagnostics/tests/multiplatform/sealedClassWithPrivateConstructor.kt");
    }

    @Test
    @TestMetadata("sealedTypeAlias.kt")
    public void testSealedTypeAlias() throws Exception {
        runTest("compiler/testData/diagnostics/tests/multiplatform/sealedTypeAlias.kt");
    }

    @Test
    @TestMetadata("sealedTypeAliasTopLevel.kt")
    public void testSealedTypeAliasTopLevel() throws Exception {
        runTest("compiler/testData/diagnostics/tests/multiplatform/sealedTypeAliasTopLevel.kt");
    }

    @Test
    @TestMetadata("smartcastOnMemberPropertyFromCommonClass.kt")
    public void testSmartcastOnMemberPropertyFromCommonClass() throws Exception {
        runTest("compiler/testData/diagnostics/tests/multiplatform/smartcastOnMemberPropertyFromCommonClass.kt");
    }

    @Test
    @TestMetadata("supertypeActualizationWithAny.kt")
    public void testSupertypeActualizationWithAny() throws Exception {
        runTest("compiler/testData/diagnostics/tests/multiplatform/supertypeActualizationWithAny.kt");
    }

    @Test
    @TestMetadata("varSetterVisibility.kt")
    public void testVarSetterVisibility() throws Exception {
        runTest("compiler/testData/diagnostics/tests/multiplatform/varSetterVisibility.kt");
    }

    @Test
    @TestMetadata("widerVisibilityInActualClassifier.kt")
    public void testWiderVisibilityInActualClassifier() throws Exception {
        runTest("compiler/testData/diagnostics/tests/multiplatform/widerVisibilityInActualClassifier.kt");
    }

    @Nested
    @TestMetadata("compiler/testData/diagnostics/tests/multiplatform/defaultArguments")
    @TestDataPath("$PROJECT_ROOT")
    public class DefaultArguments {
        @Test
        public void testAllFilesPresentInDefaultArguments() throws Exception {
            KtTestUtil.assertAllTestsPresentByMetadataWithExcluded(this.getClass(), new File("compiler/testData/diagnostics/tests/multiplatform/defaultArguments"), Pattern.compile("^(.*)\\.kts?$"), Pattern.compile("^(.+)\\.(reversed|fir|ll)\\.kts?$"), TargetBackend.JVM_IR, true);
        }

        @Test
        @TestMetadata("annotationArgumentEquality.kt")
        public void testAnnotationArgumentEquality() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/defaultArguments/annotationArgumentEquality.kt");
        }

        @Test
        @TestMetadata("annotations.kt")
        public void testAnnotations() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/defaultArguments/annotations.kt");
        }

        @Test
        @TestMetadata("annotationsViaActualTypeAlias.kt")
        public void testAnnotationsViaActualTypeAlias() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/defaultArguments/annotationsViaActualTypeAlias.kt");
        }

        @Test
        @TestMetadata("annotationsViaActualTypeAlias2.kt")
        public void testAnnotationsViaActualTypeAlias2() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/defaultArguments/annotationsViaActualTypeAlias2.kt");
        }

        @Test
        @TestMetadata("constructor.kt")
        public void testConstructor() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/defaultArguments/constructor.kt");
        }

        @Test
        @TestMetadata("constructorDefaultArgsViaActualTypealias.kt")
        public void testConstructorDefaultArgsViaActualTypealias() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/defaultArguments/constructorDefaultArgsViaActualTypealias.kt");
        }

        @Test
        @TestMetadata("expectedDeclaresDefaultArguments.kt")
        public void testExpectedDeclaresDefaultArguments() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/defaultArguments/expectedDeclaresDefaultArguments.kt");
        }

        @Test
        @TestMetadata("expectedInheritsDefaultArguments.kt")
        public void testExpectedInheritsDefaultArguments() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/defaultArguments/expectedInheritsDefaultArguments.kt");
        }

        @Test
        @TestMetadata("expectedVsNonExpectedWithDefaults.kt")
        public void testExpectedVsNonExpectedWithDefaults() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/defaultArguments/expectedVsNonExpectedWithDefaults.kt");
        }

        @Test
        @TestMetadata("methodDefaultArgsViaActualTypealias.kt")
        public void testMethodDefaultArgsViaActualTypealias() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/defaultArguments/methodDefaultArgsViaActualTypealias.kt");
        }
    }

    @Nested
    @TestMetadata("compiler/testData/diagnostics/tests/multiplatform/deprecated")
    @TestDataPath("$PROJECT_ROOT")
    public class Deprecated {
        @Test
        public void testAllFilesPresentInDeprecated() throws Exception {
            KtTestUtil.assertAllTestsPresentByMetadataWithExcluded(this.getClass(), new File("compiler/testData/diagnostics/tests/multiplatform/deprecated"), Pattern.compile("^(.*)\\.kts?$"), Pattern.compile("^(.+)\\.(reversed|fir|ll)\\.kts?$"), TargetBackend.JVM_IR, true);
        }

        @Test
        @TestMetadata("header.kt")
        public void testHeader() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/deprecated/header.kt");
        }
    }

    @Nested
    @TestMetadata("compiler/testData/diagnostics/tests/multiplatform/enum")
    @TestDataPath("$PROJECT_ROOT")
    public class Enum {
        @Test
        @TestMetadata("additionalEntriesInImpl.kt")
        public void testAdditionalEntriesInImpl() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/enum/additionalEntriesInImpl.kt");
        }

        @Test
        public void testAllFilesPresentInEnum() throws Exception {
            KtTestUtil.assertAllTestsPresentByMetadataWithExcluded(this.getClass(), new File("compiler/testData/diagnostics/tests/multiplatform/enum"), Pattern.compile("^(.*)\\.kts?$"), Pattern.compile("^(.+)\\.(reversed|fir|ll)\\.kts?$"), TargetBackend.JVM_IR, true);
        }

        @Test
        @TestMetadata("constructorInHeaderEnum.kt")
        public void testConstructorInHeaderEnum() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/enum/constructorInHeaderEnum.kt");
        }

        @Test
        @TestMetadata("differentEntryOrder.kt")
        public void testDifferentEntryOrder() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/enum/differentEntryOrder.kt");
        }

        @Test
        @TestMetadata("enumEntryWithBody.kt")
        public void testEnumEntryWithBody() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/enum/enumEntryWithBody.kt");
        }

        @Test
        @TestMetadata("javaEnum.kt")
        public void testJavaEnum() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/enum/javaEnum.kt");
        }

        @Test
        @TestMetadata("simpleEnum.kt")
        public void testSimpleEnum() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/enum/simpleEnum.kt");
        }
    }

    @Nested
    @TestMetadata("compiler/testData/diagnostics/tests/multiplatform/exhaustiveness")
    @TestDataPath("$PROJECT_ROOT")
    public class Exhaustiveness {
        @Test
        public void testAllFilesPresentInExhaustiveness() throws Exception {
            KtTestUtil.assertAllTestsPresentByMetadataWithExcluded(this.getClass(), new File("compiler/testData/diagnostics/tests/multiplatform/exhaustiveness"), Pattern.compile("^(.*)\\.kts?$"), Pattern.compile("^(.+)\\.(reversed|fir|ll)\\.kts?$"), TargetBackend.JVM_IR, true);
        }

        @Test
        @TestMetadata("commonSealedWithPlatformInheritor.kt")
        public void testCommonSealedWithPlatformInheritor() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/exhaustiveness/commonSealedWithPlatformInheritor.kt");
        }

        @Test
        @TestMetadata("expectEnum.kt")
        public void testExpectEnum() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/exhaustiveness/expectEnum.kt");
        }

        @Test
        @TestMetadata("expectSealedClass.kt")
        public void testExpectSealedClass() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/exhaustiveness/expectSealedClass.kt");
        }

        @Test
        @TestMetadata("expectSealedClassWithActualTypealias.kt")
        public void testExpectSealedClassWithActualTypealias() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/exhaustiveness/expectSealedClassWithActualTypealias.kt");
        }

        @Test
        @TestMetadata("expectSealedInterface.kt")
        public void testExpectSealedInterface() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/exhaustiveness/expectSealedInterface.kt");
        }

        @Test
        @TestMetadata("kt45796.kt")
        public void testKt45796() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/exhaustiveness/kt45796.kt");
        }
    }

    @Nested
    @TestMetadata("compiler/testData/diagnostics/tests/multiplatform/generic")
    @TestDataPath("$PROJECT_ROOT")
    public class Generic {
        @Test
        public void testAllFilesPresentInGeneric() throws Exception {
            KtTestUtil.assertAllTestsPresentByMetadataWithExcluded(this.getClass(), new File("compiler/testData/diagnostics/tests/multiplatform/generic"), Pattern.compile("^(.*)\\.kts?$"), Pattern.compile("^(.+)\\.(reversed|fir|ll)\\.kts?$"), TargetBackend.JVM_IR, true);
        }

        @Test
        @TestMetadata("functionTypeParameterBounds.kt")
        public void testFunctionTypeParameterBounds() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/generic/functionTypeParameterBounds.kt");
        }

        @Test
        @TestMetadata("genericMemberBounds.kt")
        public void testGenericMemberBounds() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/generic/genericMemberBounds.kt");
        }

        @Test
        @TestMetadata("membersInGenericClass.kt")
        public void testMembersInGenericClass() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/generic/membersInGenericClass.kt");
        }

        @Test
        @TestMetadata("typeParameterBoundsDifferentOrderActualMissing.kt")
        public void testTypeParameterBoundsDifferentOrderActualMissing() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/generic/typeParameterBoundsDifferentOrderActualMissing.kt");
        }
    }

    @Nested
    @TestMetadata("compiler/testData/diagnostics/tests/multiplatform/headerClass")
    @TestDataPath("$PROJECT_ROOT")
    public class HeaderClass {
        @Test
        @TestMetadata("actualClassWithDefaultValuesInAnnotationViaTypealias.kt")
        public void testActualClassWithDefaultValuesInAnnotationViaTypealias() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/headerClass/actualClassWithDefaultValuesInAnnotationViaTypealias.kt");
        }

        @Test
        @TestMetadata("actualClassWithDifferentConstructors.kt")
        public void testActualClassWithDifferentConstructors() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/headerClass/actualClassWithDifferentConstructors.kt");
        }

        @Test
        @TestMetadata("actualMethodInExpectClass.kt")
        public void testActualMethodInExpectClass() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/headerClass/actualMethodInExpectClass.kt");
        }

        @Test
        @TestMetadata("actualMissing.kt")
        public void testActualMissing() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/headerClass/actualMissing.kt");
        }

        @Test
        public void testAllFilesPresentInHeaderClass() throws Exception {
            KtTestUtil.assertAllTestsPresentByMetadataWithExcluded(this.getClass(), new File("compiler/testData/diagnostics/tests/multiplatform/headerClass"), Pattern.compile("^(.*)\\.kts?$"), Pattern.compile("^(.+)\\.(reversed|fir|ll)\\.kts?$"), TargetBackend.JVM_IR, true);
        }

        @Test
        @TestMetadata("baseExpectClassWithoutConstructor.kt")
        public void testBaseExpectClassWithoutConstructor() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/headerClass/baseExpectClassWithoutConstructor.kt");
        }

        @Test
        @TestMetadata("classKinds.kt")
        public void testClassKinds() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/headerClass/classKinds.kt");
        }

        @Test
        @TestMetadata("dontOverrideMethodsFromInterfaceInCommonCode.kt")
        public void testDontOverrideMethodsFromInterfaceInCommonCode() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/headerClass/dontOverrideMethodsFromInterfaceInCommonCode.kt");
        }

        @Test
        @TestMetadata("equalsOverrideInActualInterface.kt")
        public void testEqualsOverrideInActualInterface() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/headerClass/equalsOverrideInActualInterface.kt");
        }

        @Test
        @TestMetadata("expectClassWithExplicitAbstractMember.kt")
        public void testExpectClassWithExplicitAbstractMember() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/headerClass/expectClassWithExplicitAbstractMember.kt");
        }

        @Test
        @TestMetadata("expectClassWithoutConstructor.kt")
        public void testExpectClassWithoutConstructor() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/headerClass/expectClassWithoutConstructor.kt");
        }

        @Test
        @TestMetadata("expectDeclarationWithStrongIncompatibilities.kt")
        public void testExpectDeclarationWithStrongIncompatibilities() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/headerClass/expectDeclarationWithStrongIncompatibilities.kt");
        }

        @Test
        @TestMetadata("expectDeclarationWithWeakIncompatibilities.kt")
        public void testExpectDeclarationWithWeakIncompatibilities() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/headerClass/expectDeclarationWithWeakIncompatibilities.kt");
        }

        @Test
        @TestMetadata("expectFinalActualOpen.kt")
        public void testExpectFinalActualOpen() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/headerClass/expectFinalActualOpen.kt");
        }

        @Test
        @TestMetadata("expectFunInterface.kt")
        public void testExpectFunInterface() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/headerClass/expectFunInterface.kt");
        }

        @Test
        @TestMetadata("explicitConstructorDelegation.kt")
        public void testExplicitConstructorDelegation() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/headerClass/explicitConstructorDelegation.kt");
        }

        @Test
        @TestMetadata("extendExpectedClassWithAbstractMember.kt")
        public void testExtendExpectedClassWithAbstractMember() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/headerClass/extendExpectedClassWithAbstractMember.kt");
        }

        @Test
        @TestMetadata("extendExpectedClassWithoutExplicitOverrideOfMethod.kt")
        public void testExtendExpectedClassWithoutExplicitOverrideOfMethod() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/headerClass/extendExpectedClassWithoutExplicitOverrideOfMethod.kt");
        }

        @Test
        @TestMetadata("extraHeaderOnMembers.kt")
        public void testExtraHeaderOnMembers() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/headerClass/extraHeaderOnMembers.kt");
        }

        @Test
        @TestMetadata("functionAndPropertyWithSameName.kt")
        public void testFunctionAndPropertyWithSameName() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/headerClass/functionAndPropertyWithSameName.kt");
        }

        @Test
        @TestMetadata("genericClassImplTypeAlias.kt")
        public void testGenericClassImplTypeAlias() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/headerClass/genericClassImplTypeAlias.kt");
        }

        @Test
        @TestMetadata("headerClassMember.kt")
        public void testHeaderClassMember() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/headerClass/headerClassMember.kt");
        }

        @Test
        @TestMetadata("headerClassWithFunctionBody.kt")
        public void testHeaderClassWithFunctionBody() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/headerClass/headerClassWithFunctionBody.kt");
        }

        @Test
        @TestMetadata("implDataClass.kt")
        public void testImplDataClass() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/headerClass/implDataClass.kt");
        }

        @Test
        @TestMetadata("implOpenClass.kt")
        public void testImplOpenClass() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/headerClass/implOpenClass.kt");
        }

        @Test
        @TestMetadata("inheritanceByDelegationInExpectClass.kt")
        public void testInheritanceByDelegationInExpectClass() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/headerClass/inheritanceByDelegationInExpectClass.kt");
        }

        @Test
        @TestMetadata("memberPropertyKinds.kt")
        public void testMemberPropertyKinds() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/headerClass/memberPropertyKinds.kt");
        }

        @Test
        @TestMetadata("modalityCheckForExplicitAndImplicitOverride.kt")
        public void testModalityCheckForExplicitAndImplicitOverride() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/headerClass/modalityCheckForExplicitAndImplicitOverride.kt");
        }

        @Test
        @TestMetadata("morePermissiveVisibilityOnActual.kt")
        public void testMorePermissiveVisibilityOnActual() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/headerClass/morePermissiveVisibilityOnActual.kt");
        }

        @Test
        @TestMetadata("morePermissiveVisibilityOnActualViaTypeAlias.kt")
        public void testMorePermissiveVisibilityOnActualViaTypeAlias() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/headerClass/morePermissiveVisibilityOnActualViaTypeAlias.kt");
        }

        @Test
        @TestMetadata("nestedClasses.kt")
        public void testNestedClasses() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/headerClass/nestedClasses.kt");
        }

        @Test
        @TestMetadata("nestedClassesWithErrors.kt")
        public void testNestedClassesWithErrors() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/headerClass/nestedClassesWithErrors.kt");
        }

        @Test
        @TestMetadata("noImplKeywordOnMember.kt")
        public void testNoImplKeywordOnMember() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/headerClass/noImplKeywordOnMember.kt");
        }

        @Test
        @TestMetadata("privateMembers.kt")
        public void testPrivateMembers() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/headerClass/privateMembers.kt");
        }

        @Test
        @TestMetadata("simpleHeaderClass.kt")
        public void testSimpleHeaderClass() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/headerClass/simpleHeaderClass.kt");
        }

        @Test
        @TestMetadata("smartCastOnExpectClass.kt")
        public void testSmartCastOnExpectClass() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/headerClass/smartCastOnExpectClass.kt");
        }

        @Test
        @TestMetadata("superClass.kt")
        public void testSuperClass() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/headerClass/superClass.kt");
        }
    }

    @Nested
    @TestMetadata("compiler/testData/diagnostics/tests/multiplatform/hmpp")
    @TestDataPath("$PROJECT_ROOT")
    public class Hmpp {
        @Test
        public void testAllFilesPresentInHmpp() throws Exception {
            KtTestUtil.assertAllTestsPresentByMetadataWithExcluded(this.getClass(), new File("compiler/testData/diagnostics/tests/multiplatform/hmpp"), Pattern.compile("^(.*)\\.kts?$"), Pattern.compile("^(.+)\\.(reversed|fir|ll)\\.kts?$"), TargetBackend.JVM_IR, true);
        }

        @Test
        @TestMetadata("intermediateActualHasAdditionalSupertypes.kt")
        public void testIntermediateActualHasAdditionalSupertypes() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/hmpp/intermediateActualHasAdditionalSupertypes.kt");
        }

        @Test
        @TestMetadata("kt57320.kt")
        public void testKt57320() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/hmpp/kt57320.kt");
        }

        @Test
        @TestMetadata("kt-55570.kt")
        public void testKt_55570() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/hmpp/kt-55570.kt");
        }

        @Test
        @TestMetadata("simple.kt")
        public void testSimple() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/hmpp/simple.kt");
        }

        @Nested
        @TestMetadata("compiler/testData/diagnostics/tests/multiplatform/hmpp/multiplatformCompositeAnalysis")
        @TestDataPath("$PROJECT_ROOT")
        public class MultiplatformCompositeAnalysis {
            @Test
            public void testAllFilesPresentInMultiplatformCompositeAnalysis() throws Exception {
                KtTestUtil.assertAllTestsPresentByMetadataWithExcluded(this.getClass(), new File("compiler/testData/diagnostics/tests/multiplatform/hmpp/multiplatformCompositeAnalysis"), Pattern.compile("^(.*)\\.kts?$"), Pattern.compile("^(.+)\\.(reversed|fir|ll)\\.kts?$"), TargetBackend.JVM_IR, true);
            }

            @Test
            @TestMetadata("expectAndActualInTheSameModule.kt")
            public void testExpectAndActualInTheSameModule() throws Exception {
                runTest("compiler/testData/diagnostics/tests/multiplatform/hmpp/multiplatformCompositeAnalysis/expectAndActualInTheSameModule.kt");
            }

            @Test
            @TestMetadata("expectAndActualInTheSameModuleIncompatibilities.kt")
            public void testExpectAndActualInTheSameModuleIncompatibilities() throws Exception {
                runTest("compiler/testData/diagnostics/tests/multiplatform/hmpp/multiplatformCompositeAnalysis/expectAndActualInTheSameModuleIncompatibilities.kt");
            }

            @Test
            @TestMetadata("intermediateWithActualAndExpect.kt")
            public void testIntermediateWithActualAndExpect() throws Exception {
                runTest("compiler/testData/diagnostics/tests/multiplatform/hmpp/multiplatformCompositeAnalysis/intermediateWithActualAndExpect.kt");
            }

            @Test
            @TestMetadata("sealedInheritorsInComplexModuleStructure.kt")
            public void testSealedInheritorsInComplexModuleStructure() throws Exception {
                runTest("compiler/testData/diagnostics/tests/multiplatform/hmpp/multiplatformCompositeAnalysis/sealedInheritorsInComplexModuleStructure.kt");
            }
        }
    }

    @Nested
    @TestMetadata("compiler/testData/diagnostics/tests/multiplatform/inlineClasses")
    @TestDataPath("$PROJECT_ROOT")
    public class InlineClasses {
        @Test
        public void testAllFilesPresentInInlineClasses() throws Exception {
            KtTestUtil.assertAllTestsPresentByMetadataWithExcluded(this.getClass(), new File("compiler/testData/diagnostics/tests/multiplatform/inlineClasses"), Pattern.compile("^(.*)\\.kts?$"), Pattern.compile("^(.+)\\.(reversed|fir|ll)\\.kts?$"), TargetBackend.JVM_IR, true);
        }

        @Test
        @TestMetadata("expectActualInlineClass.kt")
        public void testExpectActualInlineClass() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/inlineClasses/expectActualInlineClass.kt");
        }

        @Test
        @TestMetadata("jvmInlineExpectValueClass.kt")
        public void testJvmInlineExpectValueClass() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/inlineClasses/jvmInlineExpectValueClass.kt");
        }
    }

    @Nested
    @TestMetadata("compiler/testData/diagnostics/tests/multiplatform/java")
    @TestDataPath("$PROJECT_ROOT")
    public class Java {
        @Test
        public void testAllFilesPresentInJava() throws Exception {
            KtTestUtil.assertAllTestsPresentByMetadataWithExcluded(this.getClass(), new File("compiler/testData/diagnostics/tests/multiplatform/java"), Pattern.compile("^(.*)\\.kts?$"), Pattern.compile("^(.+)\\.(reversed|fir|ll)\\.kts?$"), TargetBackend.JVM_IR, true);
        }

        @Test
        @TestMetadata("flexibleTypes.kt")
        public void testFlexibleTypes() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/java/flexibleTypes.kt");
        }

        @Test
        @TestMetadata("parameterNames.kt")
        public void testParameterNames() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/java/parameterNames.kt");
        }
    }

    @Nested
    @TestMetadata("compiler/testData/diagnostics/tests/multiplatform/topLevelFun")
    @TestDataPath("$PROJECT_ROOT")
    public class TopLevelFun {
        @Test
        public void testAllFilesPresentInTopLevelFun() throws Exception {
            KtTestUtil.assertAllTestsPresentByMetadataWithExcluded(this.getClass(), new File("compiler/testData/diagnostics/tests/multiplatform/topLevelFun"), Pattern.compile("^(.*)\\.kts?$"), Pattern.compile("^(.+)\\.(reversed|fir|ll)\\.kts?$"), TargetBackend.JVM_IR, true);
        }

        @Test
        @TestMetadata("callHeaderFun.kt")
        public void testCallHeaderFun() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/topLevelFun/callHeaderFun.kt");
        }

        @Test
        @TestMetadata("callableReferenceOnExpectFun.kt")
        public void testCallableReferenceOnExpectFun() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/topLevelFun/callableReferenceOnExpectFun.kt");
        }

        @Test
        @TestMetadata("conflictingHeaderDeclarations.kt")
        public void testConflictingHeaderDeclarations() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/topLevelFun/conflictingHeaderDeclarations.kt");
        }

        @Test
        @TestMetadata("conflictingImplDeclarations.kt")
        public void testConflictingImplDeclarations() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/topLevelFun/conflictingImplDeclarations.kt");
        }

        @Test
        @TestMetadata("functionModifiers.kt")
        public void testFunctionModifiers() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/topLevelFun/functionModifiers.kt");
        }

        @Test
        @TestMetadata("headerAndImplInDIfferentPackages.kt")
        public void testHeaderAndImplInDIfferentPackages() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/topLevelFun/headerAndImplInDIfferentPackages.kt");
        }

        @Test
        @TestMetadata("headerDeclarationWithBody.kt")
        public void testHeaderDeclarationWithBody() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/topLevelFun/headerDeclarationWithBody.kt");
        }

        @Test
        @TestMetadata("headerWithoutImpl.kt")
        public void testHeaderWithoutImpl() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/topLevelFun/headerWithoutImpl.kt");
        }

        @Test
        @TestMetadata("implDeclarationWithoutBody.kt")
        public void testImplDeclarationWithoutBody() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/topLevelFun/implDeclarationWithoutBody.kt");
        }

        @Test
        @TestMetadata("implWithoutHeader.kt")
        public void testImplWithoutHeader() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/topLevelFun/implWithoutHeader.kt");
        }

        @Test
        @TestMetadata("inlineFun.kt")
        public void testInlineFun() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/topLevelFun/inlineFun.kt");
        }

        @Test
        @TestMetadata("simpleHeaderFun.kt")
        public void testSimpleHeaderFun() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/topLevelFun/simpleHeaderFun.kt");
        }

        @Test
        @TestMetadata("valueParameterModifiers.kt")
        public void testValueParameterModifiers() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/topLevelFun/valueParameterModifiers.kt");
        }
    }

    @Nested
    @TestMetadata("compiler/testData/diagnostics/tests/multiplatform/topLevelProperty")
    @TestDataPath("$PROJECT_ROOT")
    public class TopLevelProperty {
        @Test
        public void testAllFilesPresentInTopLevelProperty() throws Exception {
            KtTestUtil.assertAllTestsPresentByMetadataWithExcluded(this.getClass(), new File("compiler/testData/diagnostics/tests/multiplatform/topLevelProperty"), Pattern.compile("^(.*)\\.kts?$"), Pattern.compile("^(.+)\\.(reversed|fir|ll)\\.kts?$"), TargetBackend.JVM_IR, true);
        }

        @Test
        @TestMetadata("differentKindsOfProperties.kt")
        public void testDifferentKindsOfProperties() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/topLevelProperty/differentKindsOfProperties.kt");
        }

        @Test
        @TestMetadata("simpleHeaderVar.kt")
        public void testSimpleHeaderVar() throws Exception {
            runTest("compiler/testData/diagnostics/tests/multiplatform/topLevelProperty/simpleHeaderVar.kt");
        }
    }
}
