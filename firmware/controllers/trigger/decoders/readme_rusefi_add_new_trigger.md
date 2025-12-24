Adding an entirely new trigger:
1. Make a new decoder files. Example for TT_4_PLUS_2:
	a. controllers/trigger/decoders/trigger_4plus2.cpp
	b. controllers/trigger/decoders/trigger_4plus2.h

2. Update "controllers/trigger/trigger.mk" --> "$(CONTROLLERS_DIR)/trigger/decoders/trigger_4plus2.cpp \ "

3. Update "controllers/trigger/decoders/trigger_structure.cpp" -->
" 	case trigger_type_e::TT_4_PLUS_2:
		initialize4Plus2(this);
		break;"

4. Update "controllers/algo/engine_types.h" --> enum "TT_4_PLUS_2"

5. Update "integration/rusefi_config.txt" --> "4+2" (edit "#define trigger_type_e_enum" line to propogate new value to rusefi.ini TS project)
 
6. if it is an entirely new trigger, which needs a new enum:
	a. 
	b. invoke "bash gen_config.sh" once you make changes to integration/rusefi_config.txt
	c. invoke "bash gen_enum_to_string.sh" to populate auto_generated_enginetypes.cpp

7. rusefi/unit_tests$ make -j8 
	Notes:
		a. for extended test report set bool printTriggerDebug = true in trigger_decoder.cpp
		b. if compile fails, make the firmware compile for stm32f407 discovery board)

8. rusefi/unit_tests$ build/rusefi_tests 
	Notes:
		a. "build/rusefi_test --gtest_list_tests" - to list the tests
		b. "build/rusefi_test --gtest_filter=Triggers*" to run only Triggers test 

9. I am unsure whether this is only needed if a new trigger just added, which needed a new enum, or always a change in trigger is implemented:
	a. rusefi/java_tools$ ./gradlew :trigger-ui:shadowJar

10. rusefi/unit_tests$ "java -jar ../java_console/trigger-ui/build/libs/trigger-ui-all.jar" - generate new trigger images (located in unit_tests/triggers)

Notes:
rusEFI always uses the trigger with more teeth for timing operations
the other is used for sync only
sync has to be established with the primary gear, this makes it sometimes necessary to use cam as primary