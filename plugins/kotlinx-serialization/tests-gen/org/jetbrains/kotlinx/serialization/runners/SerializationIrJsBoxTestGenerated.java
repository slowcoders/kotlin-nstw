/*
 * Copyright 2010-2024 JetBrains s.r.o. and Kotlin Programming Language contributors.
 * Use of this source code is governed by the Apache 2.0 license that can be found in the license/LICENSE.txt file.
 */

package org.jetbrains.kotlinx.serialization.runners;

import com.intellij.testFramework.TestDataPath;
import org.jetbrains.kotlin.test.util.KtTestUtil;
import org.jetbrains.kotlin.test.TargetBackend;
import org.jetbrains.kotlin.test.TestMetadata;
import org.junit.jupiter.api.Test;

import java.io.File;
import java.util.regex.Pattern;

/** This class is generated by {@link org.jetbrains.kotlinx.serialization.TestGeneratorKt}. DO NOT MODIFY MANUALLY */
@SuppressWarnings("all")
@TestMetadata("plugins/kotlinx-serialization/testData/boxIr")
@TestDataPath("$PROJECT_ROOT")
public class SerializationIrJsBoxTestGenerated extends AbstractSerializationIrJsBoxTest {
  @Test
  public void testAllFilesPresentInBoxIr() {
    KtTestUtil.assertAllTestsPresentByMetadataWithExcluded(this.getClass(), new File("plugins/kotlinx-serialization/testData/boxIr"), Pattern.compile("^(.+)\\.kt$"), null, TargetBackend.JS, true);
  }

  @Test
  @TestMetadata("annotationsOnFile.kt")
  public void testAnnotationsOnFile() {
    runTest("plugins/kotlinx-serialization/testData/boxIr/annotationsOnFile.kt");
  }

  @Test
  @TestMetadata("annotationsOnFileExplicitArray.kt")
  public void testAnnotationsOnFileExplicitArray() {
    runTest("plugins/kotlinx-serialization/testData/boxIr/annotationsOnFileExplicitArray.kt");
  }

  @Test
  @TestMetadata("caching.kt")
  public void testCaching() {
    runTest("plugins/kotlinx-serialization/testData/boxIr/caching.kt");
  }

  @Test
  @TestMetadata("clashBetweenSerializableAndNonSerializableProperty.kt")
  public void testClashBetweenSerializableAndNonSerializableProperty() {
    runTest("plugins/kotlinx-serialization/testData/boxIr/clashBetweenSerializableAndNonSerializableProperty.kt");
  }

  @Test
  @TestMetadata("classSerializerAsObject.kt")
  public void testClassSerializerAsObject() {
    runTest("plugins/kotlinx-serialization/testData/boxIr/classSerializerAsObject.kt");
  }

  @Test
  @TestMetadata("constValInSerialName.kt")
  public void testConstValInSerialName() {
    runTest("plugins/kotlinx-serialization/testData/boxIr/constValInSerialName.kt");
  }

  @Test
  @TestMetadata("contextualByDefault.kt")
  public void testContextualByDefault() {
    runTest("plugins/kotlinx-serialization/testData/boxIr/contextualByDefault.kt");
  }

  @Test
  @TestMetadata("contextualFallback.kt")
  public void testContextualFallback() {
    runTest("plugins/kotlinx-serialization/testData/boxIr/contextualFallback.kt");
  }

  @Test
  @TestMetadata("contextualWithTypeParameters.kt")
  public void testContextualWithTypeParameters() {
    runTest("plugins/kotlinx-serialization/testData/boxIr/contextualWithTypeParameters.kt");
  }

  @Test
  @TestMetadata("delegatedInterface.kt")
  public void testDelegatedInterface() {
    runTest("plugins/kotlinx-serialization/testData/boxIr/delegatedInterface.kt");
  }

  @Test
  @TestMetadata("delegatedProperty.kt")
  public void testDelegatedProperty() {
    runTest("plugins/kotlinx-serialization/testData/boxIr/delegatedProperty.kt");
  }

  @Test
  @TestMetadata("enumsAreCached.kt")
  public void testEnumsAreCached() {
    runTest("plugins/kotlinx-serialization/testData/boxIr/enumsAreCached.kt");
  }

  @Test
  @TestMetadata("genericBaseClassMultiple.kt")
  public void testGenericBaseClassMultiple() {
    runTest("plugins/kotlinx-serialization/testData/boxIr/genericBaseClassMultiple.kt");
  }

  @Test
  @TestMetadata("genericBaseClassSimple.kt")
  public void testGenericBaseClassSimple() {
    runTest("plugins/kotlinx-serialization/testData/boxIr/genericBaseClassSimple.kt");
  }

  @Test
  @TestMetadata("generics.kt")
  public void testGenerics() {
    runTest("plugins/kotlinx-serialization/testData/boxIr/generics.kt");
  }

  @Test
  @TestMetadata("interfaces.kt")
  public void testInterfaces() {
    runTest("plugins/kotlinx-serialization/testData/boxIr/interfaces.kt");
  }

  @Test
  @TestMetadata("KeepGeneratedSerializer.kt")
  public void testKeepGeneratedSerializer() {
    runTest("plugins/kotlinx-serialization/testData/boxIr/KeepGeneratedSerializer.kt");
  }

  @Test
  @TestMetadata("metaSerializable.kt")
  public void testMetaSerializable() {
    runTest("plugins/kotlinx-serialization/testData/boxIr/metaSerializable.kt");
  }

  @Test
  @TestMetadata("multipleGenericsPolymorphic.kt")
  public void testMultipleGenericsPolymorphic() {
    runTest("plugins/kotlinx-serialization/testData/boxIr/multipleGenericsPolymorphic.kt");
  }

  @Test
  @TestMetadata("sealedClassMultifile.kt")
  public void testSealedClassMultifile() {
    runTest("plugins/kotlinx-serialization/testData/boxIr/sealedClassMultifile.kt");
  }

  @Test
  @TestMetadata("sealedInterfaces.kt")
  public void testSealedInterfaces() {
    runTest("plugins/kotlinx-serialization/testData/boxIr/sealedInterfaces.kt");
  }

  @Test
  @TestMetadata("serializableCompanion.kt")
  public void testSerializableCompanion() {
    runTest("plugins/kotlinx-serialization/testData/boxIr/serializableCompanion.kt");
  }

  @Test
  @TestMetadata("serializableFromAnotherModule.kt")
  public void testSerializableFromAnotherModule() {
    runTest("plugins/kotlinx-serialization/testData/boxIr/serializableFromAnotherModule.kt");
  }

  @Test
  @TestMetadata("serializableFromAnotherModule_multipleFields.kt")
  public void testSerializableFromAnotherModule_multipleFields() {
    runTest("plugins/kotlinx-serialization/testData/boxIr/serializableFromAnotherModule_multipleFields.kt");
  }

  @Test
  @TestMetadata("serializableOnPropertyType.kt")
  public void testSerializableOnPropertyType() {
    runTest("plugins/kotlinx-serialization/testData/boxIr/serializableOnPropertyType.kt");
  }

  @Test
  @TestMetadata("serializerFactory.kt")
  public void testSerializerFactory() {
    runTest("plugins/kotlinx-serialization/testData/boxIr/serializerFactory.kt");
  }

  @Test
  @TestMetadata("serializerFactoryInUserDefined.kt")
  public void testSerializerFactoryInUserDefined() {
    runTest("plugins/kotlinx-serialization/testData/boxIr/serializerFactoryInUserDefined.kt");
  }

  @Test
  @TestMetadata("starProjections.kt")
  public void testStarProjections() {
    runTest("plugins/kotlinx-serialization/testData/boxIr/starProjections.kt");
  }

  @Test
  @TestMetadata("useSerializersChain.kt")
  public void testUseSerializersChain() {
    runTest("plugins/kotlinx-serialization/testData/boxIr/useSerializersChain.kt");
  }

  @Test
  @TestMetadata("userDefinedSerializerInCompanion.kt")
  public void testUserDefinedSerializerInCompanion() {
    runTest("plugins/kotlinx-serialization/testData/boxIr/userDefinedSerializerInCompanion.kt");
  }

  @Test
  @TestMetadata("uuidSerializer.kt")
  public void testUuidSerializer() {
    runTest("plugins/kotlinx-serialization/testData/boxIr/uuidSerializer.kt");
  }
}
