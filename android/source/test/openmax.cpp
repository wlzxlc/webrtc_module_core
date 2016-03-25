#include <OMX_Types.h>
#include <OMX_Component.h>
#include <stdio.h>
int main(int c, char **s){
 printf("OMX_U32=%d uint32_t %d OMX_PTR %d\n", sizeof(OMX_U32),sizeof(uint32_t), sizeof(OMX_PTR));
 printf("OMX_PARAM_PORTDEFINITIONTYPE=%d\n",sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
 return 0;
}
