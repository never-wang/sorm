
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <CUnit/Basic.h>

int main(int args, char *argv[])
{
	CU_BasicRunMode mode = CU_BRM_VERBOSE;
	CU_ErrorAction error_action = CUEA_FAIL;

	setvbuf(stdout, 0, _IONBF, 0);

	if (CU_initialize_registry()) {
		printf("\nTest init error\n");
	} else {
		AddTests();
		CU_basic_set_mode(mode);
		CU_set_error_action(error_action);
		printf("\nTests completed with return value %d. \n", CU_basic_run_tests());
		CU_cleanup_registry();
	}
}


