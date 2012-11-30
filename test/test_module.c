#include <stdio.h>
#include "module.h"

static int module_init(void)
{
	printf("This is the test module init function\n");
	return 0;
}

static void module_exit(void)
{
	printf("This is the test module exit function\n");
}

MODULE(test_module, module_init, module_exit);
