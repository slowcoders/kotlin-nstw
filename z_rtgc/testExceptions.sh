clear && printf '\e[3J'
sh z_rtgc/exec_test.sh cleaner_basic
sh z_rtgc/exec_test.sh cycle_detector
sh z_rtgc/exec_test.sh freeze_stress
sh z_rtgc/exec_test.sh interop_objc_tests
sh z_rtgc/exec_test.sh testObjCExport
sh z_rtgc/exec_test.sh exception_in_global_init
sh z_rtgc/exec_test.sh lazy3
sh z_rtgc/exec_test.sh interop_cleaners_leak
sh z_rtgc/exec_test.sh interop_kotlin_exception_hook
sh z_rtgc/exec_test.sh worker_bound_reference0
sh z_rtgc/exec_test.sh custom_hook_unhandled_exception
sh z_rtgc/exec_test.sh custom_hook_terminate_unhandled_exception
sh z_rtgc/exec_test.sh interop_exceptions_cCallback
sh z_rtgc/exec_test.sh worker_exceptions_terminate_hook_current
sh z_rtgc/exec_test.sh worker_exceptions_terminate_current_legacy
sh z_rtgc/exec_test.sh cleaner_workers
sh z_rtgc/exec_test.sh resultsTask
sh z_rtgc/exec_test.sh harmonyRegexTest
sh z_rtgc/exec_test.sh codegen_escapeAnalysis_stackAllocated # disorder of frame address
sh z_rtgc/exec_test.sh kt36639 # error on opt build. (ObjHolder's frame address order is corrupted)
sh z_rtgc/exec_test.sh leakMemoryWithTestRunner
sh z_rtgc/exec_test.sh hello0 && ./test.output/local/macos_x64/localTest.kexe
