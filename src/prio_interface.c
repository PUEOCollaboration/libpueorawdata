#include <pueo/prio_interface.h>
#include <dlfcn.h>


#define PUEO_PRIO_LOAD(ret,name,args)\
  impl->name = dlsym(so,#name);\
  if (!impl->name) return retval++;


int  pueo_prio_create(const char * sopath, pueo_prio_impl_t * impl)
{
  void * so = dlopen(sopath, RTLD_LAZY);
  int retval = 1;
  if (!so) return retval++;

  PUEO_PRIO_FUNCTIONS(PUEO_PRIO_LOAD)

  return 0;
}
