/*
 * Copyright 2010-2024 JetBrains s.r.o. and Kotlin Programming Language contributors.
 * Use of this source code is governed by the Apache 2.0 license that can be found in the license/LICENSE.txt file.
 */

package org.jetbrains.kotlin.js.test.ir;

import com.intellij.testFramework.TestDataPath;
import org.jetbrains.kotlin.test.util.KtTestUtil;
import org.jetbrains.kotlin.test.TargetBackend;
import org.jetbrains.kotlin.test.TestMetadata;
import org.junit.jupiter.api.Nested;
import org.junit.jupiter.api.Test;

import java.io.File;
import java.util.regex.Pattern;

/** This class is generated by {@link org.jetbrains.kotlin.generators.tests.GenerateJsTestsKt}. DO NOT MODIFY MANUALLY */
@SuppressWarnings("all")
@TestMetadata("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation")
@TestDataPath("$PROJECT_ROOT")
public class DiagnosticsWithJsStdLibAndBackendTestGenerated extends AbstractDiagnosticsTestWithJsStdLibWithBackend {
  @Test
  public void testAllFilesPresentInTestsWithJsStdLibAndBackendCompilation() {
    KtTestUtil.assertAllTestsPresentByMetadataWithExcluded(this.getClass(), new File("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation"), Pattern.compile("^([^_](.+))\\.kt$"), Pattern.compile("^(.+)\\.fir\\.kts?$"), TargetBackend.JS, true);
  }

  @Nested
  @TestMetadata("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/constants")
  @TestDataPath("$PROJECT_ROOT")
  public class Constants {
    @Test
    public void testAllFilesPresentInConstants() {
      KtTestUtil.assertAllTestsPresentByMetadataWithExcluded(this.getClass(), new File("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/constants"), Pattern.compile("^([^_](.+))\\.kt$"), Pattern.compile("^(.+)\\.fir\\.kts?$"), TargetBackend.JS, true);
    }

    @Test
    @TestMetadata("simpleConstFromLib.kt")
    public void testSimpleConstFromLib() {
      runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/constants/simpleConstFromLib.kt");
    }
  }

  @Nested
  @TestMetadata("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/exportedNamesClash")
  @TestDataPath("$PROJECT_ROOT")
  public class ExportedNamesClash {
    @Test
    public void testAllFilesPresentInExportedNamesClash() {
      KtTestUtil.assertAllTestsPresentByMetadataWithExcluded(this.getClass(), new File("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/exportedNamesClash"), Pattern.compile("^([^_](.+))\\.kt$"), Pattern.compile("^(.+)\\.fir\\.kts?$"), TargetBackend.JS, true);
    }

    @Test
    @TestMetadata("functionAndFunction.kt")
    public void testFunctionAndFunction() {
      runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/exportedNamesClash/functionAndFunction.kt");
    }

    @Test
    @TestMetadata("functionAndProperty.kt")
    public void testFunctionAndProperty() {
      runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/exportedNamesClash/functionAndProperty.kt");
    }

    @Test
    @TestMetadata("functionDifferentPackages.kt")
    public void testFunctionDifferentPackages() {
      runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/exportedNamesClash/functionDifferentPackages.kt");
    }

    @Test
    @TestMetadata("multipleClash.kt")
    public void testMultipleClash() {
      runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/exportedNamesClash/multipleClash.kt");
    }

    @Test
    @TestMetadata("multipleClashESModules.kt")
    public void testMultipleClashESModules() {
      runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/exportedNamesClash/multipleClashESModules.kt");
    }

    @Test
    @TestMetadata("packageAndFunction.kt")
    public void testPackageAndFunction() {
      runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/exportedNamesClash/packageAndFunction.kt");
    }

    @Test
    @TestMetadata("packageAndProperty.kt")
    public void testPackageAndProperty() {
      runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/exportedNamesClash/packageAndProperty.kt");
    }

    @Test
    @TestMetadata("propertyAndProperty.kt")
    public void testPropertyAndProperty() {
      runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/exportedNamesClash/propertyAndProperty.kt");
    }

    @Test
    @TestMetadata("propertyDifferentPackages.kt")
    public void testPropertyDifferentPackages() {
      runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/exportedNamesClash/propertyDifferentPackages.kt");
    }
  }

  @Nested
  @TestMetadata("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/inline")
  @TestDataPath("$PROJECT_ROOT")
  public class Inline {
    @Test
    public void testAllFilesPresentInInline() {
      KtTestUtil.assertAllTestsPresentByMetadataWithExcluded(this.getClass(), new File("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/inline"), Pattern.compile("^([^_](.+))\\.kt$"), Pattern.compile("^(.+)\\.fir\\.kts?$"), TargetBackend.JS, true);
    }

    @Test
    @TestMetadata("recursionCycle.kt")
    public void testRecursionCycle() {
      runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/inline/recursionCycle.kt");
    }

    @Test
    @TestMetadata("recursionCycleLambda.kt")
    public void testRecursionCycleLambda() {
      runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/inline/recursionCycleLambda.kt");
    }

    @Test
    @TestMetadata("recursionCycleWithPublicFun.kt")
    public void testRecursionCycleWithPublicFun() {
      runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/inline/recursionCycleWithPublicFun.kt");
    }

    @Test
    @TestMetadata("recursionCycleWithStdlibCall.kt")
    public void testRecursionCycleWithStdlibCall() {
      runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/inline/recursionCycleWithStdlibCall.kt");
    }
  }

  @Nested
  @TestMetadata("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/jsCode")
  @TestDataPath("$PROJECT_ROOT")
  public class JsCode {
    @Test
    public void testAllFilesPresentInJsCode() {
      KtTestUtil.assertAllTestsPresentByMetadataWithExcluded(this.getClass(), new File("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/jsCode"), Pattern.compile("^([^_](.+))\\.kt$"), Pattern.compile("^(.+)\\.fir\\.kts?$"), TargetBackend.JS, true);
    }

    @Test
    @TestMetadata("argumentIsLiteral.kt")
    public void testArgumentIsLiteral() {
      runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/jsCode/argumentIsLiteral.kt");
    }

    @Test
    @TestMetadata("badAssignment.kt")
    public void testBadAssignment() {
      runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/jsCode/badAssignment.kt");
    }

    @Test
    @TestMetadata("compileTimeString.kt")
    public void testCompileTimeString() {
      runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/jsCode/compileTimeString.kt");
    }

    @Test
    @TestMetadata("compileTimeStringWithCompanionVal.kt")
    public void testCompileTimeStringWithCompanionVal() {
      runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/jsCode/compileTimeStringWithCompanionVal.kt");
    }

    @Test
    @TestMetadata("compileTimeStringWithConstVal.kt")
    public void testCompileTimeStringWithConstVal() {
      runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/jsCode/compileTimeStringWithConstVal.kt");
    }

    @Test
    @TestMetadata("compileTimeStringWithFunCall.kt")
    public void testCompileTimeStringWithFunCall() {
      runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/jsCode/compileTimeStringWithFunCall.kt");
    }

    @Test
    @TestMetadata("compileTimeStringWithIntrinConstCall.kt")
    public void testCompileTimeStringWithIntrinConstCall() {
      runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/jsCode/compileTimeStringWithIntrinConstCall.kt");
    }

    @Test
    @TestMetadata("compileTimeStringWithTopLevelVal.kt")
    public void testCompileTimeStringWithTopLevelVal() {
      runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/jsCode/compileTimeStringWithTopLevelVal.kt");
    }

    @Test
    @TestMetadata("compileTimeStringWithVal.kt")
    public void testCompileTimeStringWithVal() {
      runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/jsCode/compileTimeStringWithVal.kt");
    }

    @Test
    @TestMetadata("deleteOperation.kt")
    public void testDeleteOperation() {
      runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/jsCode/deleteOperation.kt");
    }

    @Test
    @TestMetadata("error.kt")
    public void testError() {
      runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/jsCode/error.kt");
    }

    @Test
    @TestMetadata("javaScriptEmpty.kt")
    public void testJavaScriptEmpty() {
      runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/jsCode/javaScriptEmpty.kt");
    }

    @Test
    @TestMetadata("javaScriptError.kt")
    public void testJavaScriptError() {
      runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/jsCode/javaScriptError.kt");
    }

    @Test
    @TestMetadata("javaScriptWarning.kt")
    public void testJavaScriptWarning() {
      runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/jsCode/javaScriptWarning.kt");
    }

    @Test
    @TestMetadata("noJavaScriptProduced.kt")
    public void testNoJavaScriptProduced() {
      runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/jsCode/noJavaScriptProduced.kt");
    }

    @Test
    @TestMetadata("warning.kt")
    public void testWarning() {
      runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/jsCode/warning.kt");
    }

    @Nested
    @TestMetadata("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/jsCode/inlinedReturnBreakContinue")
    @TestDataPath("$PROJECT_ROOT")
    public class InlinedReturnBreakContinue {
      @Test
      public void testAllFilesPresentInInlinedReturnBreakContinue() {
        KtTestUtil.assertAllTestsPresentByMetadataWithExcluded(this.getClass(), new File("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/jsCode/inlinedReturnBreakContinue"), Pattern.compile("^([^_](.+))\\.kt$"), Pattern.compile("^(.+)\\.fir\\.kts?$"), TargetBackend.JS, true);
      }

      @Test
      @TestMetadata("inlineFunctionWithMultipleParameters.kt")
      public void testInlineFunctionWithMultipleParameters() {
        runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/jsCode/inlinedReturnBreakContinue/inlineFunctionWithMultipleParameters.kt");
      }

      @Test
      @TestMetadata("lambdaPassedToInlineFunction.kt")
      public void testLambdaPassedToInlineFunction() {
        runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/jsCode/inlinedReturnBreakContinue/lambdaPassedToInlineFunction.kt");
      }

      @Test
      @TestMetadata("loopWithinInlineFunction.kt")
      public void testLoopWithinInlineFunction() {
        runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/jsCode/inlinedReturnBreakContinue/loopWithinInlineFunction.kt");
      }

      @Test
      @TestMetadata("nonLocalReturn.kt")
      public void testNonLocalReturn() {
        runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/jsCode/inlinedReturnBreakContinue/nonLocalReturn.kt");
      }

      @Test
      @TestMetadata("simple.kt")
      public void testSimple() {
        runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/jsCode/inlinedReturnBreakContinue/simple.kt");
      }

      @Test
      @TestMetadata("simpleDoWhile.kt")
      public void testSimpleDoWhile() {
        runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/jsCode/inlinedReturnBreakContinue/simpleDoWhile.kt");
      }

      @Test
      @TestMetadata("withReturnValue.kt")
      public void testWithReturnValue() {
        runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/jsCode/inlinedReturnBreakContinue/withReturnValue.kt");
      }

      @Test
      @TestMetadata("withReturnValueDoWhileContinue.kt")
      public void testWithReturnValueDoWhileContinue() {
        runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/jsCode/inlinedReturnBreakContinue/withReturnValueDoWhileContinue.kt");
      }

      @Test
      @TestMetadata("withReturnValueNested.kt")
      public void testWithReturnValueNested() {
        runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/jsCode/inlinedReturnBreakContinue/withReturnValueNested.kt");
      }
    }
  }

  @Nested
  @TestMetadata("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/name")
  @TestDataPath("$PROJECT_ROOT")
  public class Name {
    @Test
    public void testAllFilesPresentInName() {
      KtTestUtil.assertAllTestsPresentByMetadataWithExcluded(this.getClass(), new File("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/name"), Pattern.compile("^([^_](.+))\\.kt$"), Pattern.compile("^(.+)\\.fir\\.kts?$"), TargetBackend.JS, true);
    }

    @Test
    @TestMetadata("illegalNameIR.kt")
    public void testIllegalNameIR() {
      runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/name/illegalNameIR.kt");
    }

    @Test
    @TestMetadata("legalPackageName.kt")
    public void testLegalPackageName() {
      runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/name/legalPackageName.kt");
    }
  }

  @Nested
  @TestMetadata("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/unsupportedFeatures")
  @TestDataPath("$PROJECT_ROOT")
  public class UnsupportedFeatures {
    @Test
    public void testAllFilesPresentInUnsupportedFeatures() {
      KtTestUtil.assertAllTestsPresentByMetadataWithExcluded(this.getClass(), new File("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/unsupportedFeatures"), Pattern.compile("^([^_](.+))\\.kt$"), Pattern.compile("^(.+)\\.fir\\.kts?$"), TargetBackend.JS, true);
    }

    @Test
    @TestMetadata("annotations.kt")
    public void testAnnotations() {
      runTest("compiler/testData/diagnostics/testsWithJsStdLibAndBackendCompilation/unsupportedFeatures/annotations.kt");
    }
  }
}
