const binding = process._linkedBinding('electron_common_testing');
binding.log(1, 'CHILD_PROCESS_TEST_LOG');
binding.log(1, `CHILD_PROCESS_DESTINATION_${binding.getLoggingDestination()}`);
