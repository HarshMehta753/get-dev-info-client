#include "../inc/define.h"

int main(int argc, char *argv[])
{
	printf("The client process has started.\r\n");

	if (client_init() == false)
	{
		printf("Error: the client process has init failed.\r\n");
		return -1;
	}

	printf("Starting client application.\r\n");
	printf("Waiting for server request.\r\n");
	client_process();

	return 0;
}

