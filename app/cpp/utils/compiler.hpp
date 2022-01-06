#pragma once

#ifndef HAS_BUILTIN
#define HAS_BUILTIN(x) 0
#endif

#if HAS_BUILTIN(__builtin_expect)
#   ifdef __cplusplus
#      define UTILS_LIKELY( exp )    (__builtin_expect( !!(exp), true ))
#      define UTILS_UNLIKELY( exp )  (__builtin_expect( !!(exp), false ))
#   else
#      define UTILS_LIKELY( exp )    (__builtin_expect( !!(exp), 1 ))
#      define UTILS_UNLIKELY( exp )  (__builtin_expect( !!(exp), 0 ))
#   endif
#else
#   define UTILS_LIKELY(exp)    (!!(exp))
#   define UTILS_UNLIKELY(exp)  (!!(exp))
#endif
