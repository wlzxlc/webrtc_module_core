#ifndef BUG_HELPER_H
#define BUG_HELPER_H
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/syscall.h>

#define PREFIX(x) I ## x
#ifndef ERRNUM
#define ERRNUM(str,...) -__LINE__
#endif

#ifndef bug_log
#define bug_log(string) assert(string && false)
#endif

/*
 * If BUG_IGNORE by enable then should be ingore bug line.
*/
#ifdef BUG_IGNORE
//#warning "********Ingore BUG checking with skip bug's context********."
#define BUG(ID, context)
#else
/*
 * Force check bug line.
*/
#ifdef BUG_CHECK
//#warning "********Enable BUG checking********."
#define BUG(ID, context) bug_log("***BUG***ID:"#ID"==>" #context)
#else
/*
 * Default mode, do not check and do not ingore.
*/
//#warning "********Do not checked BUG list********."
#define BUG(ID, context) context
#endif
#endif

inline const char *__notdir(const char *tag)
{
  int size = strlen(tag);
  if( !size) {
      return tag;
    }
  const char *p = tag + size - 1;
  while(*p != '/' && p != tag)p--;
  if (p != tag){
      p++;
    }
  return p;
}

#ifdef DEBUG_RTC
#undef DEBUG
#define DEBUG(fmt,...) printf("%s[L%d]%s: " fmt "\n",__notdir(__FILE__),__LINE__,__FUNCTION__,## __VA_ARGS__)
#define RTC_FIND_SYM(name, handle) _##name  = \
   (typeof(_##name)) dlsym(handle, "I"#name); \
   DEBUG("%s: %p\n", "I"#name, _##name)

#define RTC_CALL(name) (\
  DEBUG("RTC_CALL: %s addr %p\n", __FUNCTION__, _##name) || \
  true )&& _##name == NULL ? -__LINE__ :(_##name)
#else
#define DEBUG(...)
#define RTC_FIND_SYM(name, handle) _##name  = \
   (typeof(_##name)) dlsym(handle, "I"#name)

#define RTC_CALL(name) _##name == NULL ? -__LINE__ :(_##name)
#endif

#ifdef __cplusplus
#include <string>
class ScopedCostTime{
private:
  struct timeval _start;
  std::string _tag;
  int _line;
public:
  ScopedCostTime(int line,const char *tag):
    _tag(tag),
    _line(line){
    gettimeofday(&_start,NULL);
  }
  ~ScopedCostTime(){
    struct timeval end;
    gettimeofday(&end,NULL);
#ifdef DEBUG_RTC
    long long interval_us = 0;
    interval_us = (end.tv_sec - _start.tv_sec)*1E6 + (end.tv_usec - _start.tv_usec);
    DEBUG("%s[L%d,tid:%u] cost time %0.2f(ms)", _tag.c_str(), _line,
          static_cast<unsigned int>((syscall(__NR_gettid))),interval_us / 1000.0);
#endif
  }
};

#ifndef SCOPEDCOSTTIME
#define SCOPEDCOSTTIME(tag) ScopedCostTime __costtime(__LINE__, tag)
#endif
#ifndef SCOPEDCOSTTIME_FUN
#define SCOPEDCOSTTIME_FUN() ScopedCostTime __costtime(__LINE__,__notdir(__FILE__))
#endif
#endif // end of __cplusplus

#endif // BUG_HELPER_H
