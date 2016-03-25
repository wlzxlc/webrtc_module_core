#include "MediaSourceService.h"

int main(int c, char **s)
{
	const char *name = "media_source_service";
	MediaSourceService *service = new MediaSourceService();
	int ret = service->init(name);
	ret = service->start();
	printf("media_source_service exit...ret %d\n", ret);
	return ret;
}