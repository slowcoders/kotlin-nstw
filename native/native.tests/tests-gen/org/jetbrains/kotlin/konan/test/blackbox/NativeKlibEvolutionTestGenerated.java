/*
 * Copyright 2010-2024 JetBrains s.r.o. and Kotlin Programming Language contributors.
 * Use of this source code is governed by the Apache 2.0 license that can be found in the license/LICENSE.txt file.
 */

package org.jetbrains.kotlin.konan.test.blackbox;

import com.intellij.testFramework.TestDataPath;
import org.jetbrains.kotlin.test.util.KtTestUtil;
import org.jetbrains.kotlin.test.TestMetadata;
import org.junit.jupiter.api.Test;

import java.io.File;
import java.util.regex.Pattern;

/** This class is generated by {@link org.jetbrains.kotlin.generators.tests.GenerateNativeTestsKt}. DO NOT MODIFY MANUALLY */
@SuppressWarnings("all")
@TestMetadata("compiler/testData/klib/evolution")
@TestDataPath("$PROJECT_ROOT")
public class NativeKlibEvolutionTestGenerated extends AbstractNativeKlibEvolutionTest {
  @Test
  @TestMetadata("addAbstractMemberBody.kt")
  public void testAddAbstractMemberBody() {
    runTest("compiler/testData/klib/evolution/addAbstractMemberBody.kt");
  }

  @Test
  @TestMetadata("addCompanionObject.kt")
  public void testAddCompanionObject() {
    runTest("compiler/testData/klib/evolution/addCompanionObject.kt");
  }

  @Test
  @TestMetadata("addCrossinline.kt")
  public void testAddCrossinline() {
    runTest("compiler/testData/klib/evolution/addCrossinline.kt");
  }

  @Test
  @TestMetadata("addDefaultImplementations.kt")
  public void testAddDefaultImplementations() {
    runTest("compiler/testData/klib/evolution/addDefaultImplementations.kt");
  }

  @Test
  @TestMetadata("addEnumClassMember.kt")
  public void testAddEnumClassMember() {
    runTest("compiler/testData/klib/evolution/addEnumClassMember.kt");
  }

  @Test
  @TestMetadata("addLateinitToVar.kt")
  public void testAddLateinitToVar() {
    runTest("compiler/testData/klib/evolution/addLateinitToVar.kt");
  }

  @Test
  @TestMetadata("addOpenToClass.kt")
  public void testAddOpenToClass() {
    runTest("compiler/testData/klib/evolution/addOpenToClass.kt");
  }

  @Test
  @TestMetadata("addOpenToMember.kt")
  public void testAddOpenToMember() {
    runTest("compiler/testData/klib/evolution/addOpenToMember.kt");
  }

  @Test
  @TestMetadata("addOrRemoveConst.kt")
  public void testAddOrRemoveConst() {
    runTest("compiler/testData/klib/evolution/addOrRemoveConst.kt");
  }

  @Test
  @TestMetadata("addOrRemoveInitBlock.kt")
  public void testAddOrRemoveInitBlock() {
    runTest("compiler/testData/klib/evolution/addOrRemoveInitBlock.kt");
  }

  @Test
  @TestMetadata("addOverloads.kt")
  public void testAddOverloads() {
    runTest("compiler/testData/klib/evolution/addOverloads.kt");
  }

  @Test
  @TestMetadata("addParameterDefaulValue.kt")
  public void testAddParameterDefaulValue() {
    runTest("compiler/testData/klib/evolution/addParameterDefaulValue.kt");
  }

  @Test
  @TestMetadata("addPropertyAccessor.kt")
  public void testAddPropertyAccessor() {
    runTest("compiler/testData/klib/evolution/addPropertyAccessor.kt");
  }

  @Test
  @TestMetadata("addingSealedClassMember.kt")
  public void testAddingSealedClassMember() {
    runTest("compiler/testData/klib/evolution/addingSealedClassMember.kt");
  }

  @Test
  public void testAllFilesPresentInEvolution() {
    KtTestUtil.assertAllTestsPresentByMetadataWithExcluded(this.getClass(), new File("compiler/testData/klib/evolution"), Pattern.compile("^(.+)\\.kt$"), null, false);
  }

  @Test
  @TestMetadata("changeBaseClassOrder.kt")
  public void testChangeBaseClassOrder() {
    runTest("compiler/testData/klib/evolution/changeBaseClassOrder.kt");
  }

  @Test
  @TestMetadata("changeCompanionToNestedObject.kt")
  public void testChangeCompanionToNestedObject() {
    runTest("compiler/testData/klib/evolution/changeCompanionToNestedObject.kt");
  }

  @Test
  @TestMetadata("changeConstInitialization.kt")
  public void testChangeConstInitialization() {
    runTest("compiler/testData/klib/evolution/changeConstInitialization.kt");
  }

  @Test
  @TestMetadata("changeFunctionReturnType.kt")
  public void testChangeFunctionReturnType() {
    runTest("compiler/testData/klib/evolution/changeFunctionReturnType.kt");
  }

  @Test
  @TestMetadata("changeNamesOfTypeParameters.kt")
  public void testChangeNamesOfTypeParameters() {
    runTest("compiler/testData/klib/evolution/changeNamesOfTypeParameters.kt");
  }

  @Test
  @TestMetadata("changeObjectToCompanion.kt")
  public void testChangeObjectToCompanion() {
    runTest("compiler/testData/klib/evolution/changeObjectToCompanion.kt");
  }

  @Test
  @TestMetadata("changeParameterDefaultValue.kt")
  public void testChangeParameterDefaultValue() {
    runTest("compiler/testData/klib/evolution/changeParameterDefaultValue.kt");
  }

  @Test
  @TestMetadata("changePropertyFromValToVar.kt")
  public void testChangePropertyFromValToVar() {
    runTest("compiler/testData/klib/evolution/changePropertyFromValToVar.kt");
  }

  @Test
  @TestMetadata("changePropertyInitialization.kt")
  public void testChangePropertyInitialization() {
    runTest("compiler/testData/klib/evolution/changePropertyInitialization.kt");
  }

  @Test
  @TestMetadata("constructorParameterMarkValVar.kt")
  public void testConstructorParameterMarkValVar() {
    runTest("compiler/testData/klib/evolution/constructorParameterMarkValVar.kt");
  }

  @Test
  @TestMetadata("deleteOverrideMember.kt")
  public void testDeleteOverrideMember() {
    runTest("compiler/testData/klib/evolution/deleteOverrideMember.kt");
  }

  @Test
  @TestMetadata("deletePrivateMembers.kt")
  public void testDeletePrivateMembers() {
    runTest("compiler/testData/klib/evolution/deletePrivateMembers.kt");
  }

  @Test
  @TestMetadata("inlineBodyChange.kt")
  public void testInlineBodyChange() {
    runTest("compiler/testData/klib/evolution/inlineBodyChange.kt");
  }

  @Test
  @TestMetadata("inlineFunction.kt")
  public void testInlineFunction() {
    runTest("compiler/testData/klib/evolution/inlineFunction.kt");
  }

  @Test
  @TestMetadata("makeFunctionInfixOrTailrec.kt")
  public void testMakeFunctionInfixOrTailrec() {
    runTest("compiler/testData/klib/evolution/makeFunctionInfixOrTailrec.kt");
  }

  @Test
  @TestMetadata("moreSpecificBaseClass.kt")
  public void testMoreSpecificBaseClass() {
    runTest("compiler/testData/klib/evolution/moreSpecificBaseClass.kt");
  }

  @Test
  @TestMetadata("moveMemberUpInHierarchy.kt")
  public void testMoveMemberUpInHierarchy() {
    runTest("compiler/testData/klib/evolution/moveMemberUpInHierarchy.kt");
  }

  @Test
  @TestMetadata("newFakeOverride.kt")
  public void testNewFakeOverride() {
    runTest("compiler/testData/klib/evolution/newFakeOverride.kt");
  }

  @Test
  @TestMetadata("newOverrideMember.kt")
  public void testNewOverrideMember() {
    runTest("compiler/testData/klib/evolution/newOverrideMember.kt");
  }

  @Test
  @TestMetadata("removeAbstractFromClass.kt")
  public void testRemoveAbstractFromClass() {
    runTest("compiler/testData/klib/evolution/removeAbstractFromClass.kt");
  }

  @Test
  @TestMetadata("removeInfixOrTailrecFromFunction.kt")
  public void testRemoveInfixOrTailrecFromFunction() {
    runTest("compiler/testData/klib/evolution/removeInfixOrTailrecFromFunction.kt");
  }

  @Test
  @TestMetadata("removeLateinitFromVar.kt")
  public void testRemoveLateinitFromVar() {
    runTest("compiler/testData/klib/evolution/removeLateinitFromVar.kt");
  }

  @Test
  @TestMetadata("removePropertyAccessor.kt")
  public void testRemovePropertyAccessor() {
    runTest("compiler/testData/klib/evolution/removePropertyAccessor.kt");
  }

  @Test
  @TestMetadata("renameArguments.kt")
  public void testRenameArguments() {
    runTest("compiler/testData/klib/evolution/renameArguments.kt");
  }

  @Test
  @TestMetadata("reorderClassConstructors.kt")
  public void testReorderClassConstructors() {
    runTest("compiler/testData/klib/evolution/reorderClassConstructors.kt");
  }

  @Test
  @TestMetadata("turnClassIntoDataClass.kt")
  public void testTurnClassIntoDataClass() {
    runTest("compiler/testData/klib/evolution/turnClassIntoDataClass.kt");
  }

  @Test
  @TestMetadata("widenSuperMemberVisibility.kt")
  public void testWidenSuperMemberVisibility() {
    runTest("compiler/testData/klib/evolution/widenSuperMemberVisibility.kt");
  }
}
