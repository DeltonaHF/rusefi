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
	a. invoke "bash ./gen_config.sh" once you make changes to integration/rusefi_config.txt - this populates .ini files for TunerStudio
	b. invoke "bash ./gen_enum_to_string.sh" to populate auto_generated_enginetypes.cpp

7. rusefi/unit_tests$ make -j8 
	Notes:
		a. for extended test report set bool printTriggerDebug = true in trigger_decoder.cpp
		b. if compile fails, make the firmware compile for stm32f407 discovery board)

8. rusefi/unit_tests$ build/rusefi_tests 
	Notes:
		a. "build/rusefi_test --gtest_list_tests" - to list the tests
		b. "build/rusefi_test --gtest_filter=Triggers*" to run only Triggers test 

9. Only needed if a new trigger just added, which needed a new enum:
	a. rusefi/java_tools$ ./gradlew :trigger-ui:shadowJar

10. rusefi/unit_tests$ "java -jar ../java_console/trigger-ui/build/libs/trigger-ui-all.jar" - generate new trigger images (located in unit_tests/triggers)

-----
VVT
1. add as a regular decoder (for camshaft, and w/o tdc offset)
2. add to trigger_structure switch case (unsure if this is needed, when wheel only used for VVT)
3. controllers/algo/engine_types.h --> enum for trigger structure switch case
4. firmware/controllers/algo/rusefi_enums.h --> append add enum for vvt
4. firmware/controllers/algo/engine.cpp --> add mapping between vvt_mode_e and trigger_type_e in getVvtTriggerType (e.g. case VVT_4_MINUS_2: return rigger_type_e::TT_4_MINUS_2_VVT;)

 

Notes:
WRONG ASSUMPTION:
[rusEFI always uses the trigger with more teeth for timing operations
the other is used for sync only
sync has to be established with the primary gear, this makes it sometimes necessary to use cam as primary]