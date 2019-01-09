#ifndef DB_EX1_ATTRIBUTES_H
#define DB_EX1_ATTRIBUTES_H

#ifdef __GNUC__

#define __NON_NULL(...) __attribute__((nonnull(__VA_ARGS__)))
#define __NO_DISCARD __attribute__((warn_unused_result))
#define __INLINE __attribute__((always_inline))

#else

#define __NON_NULL
#define __NO_DISCARD
#define __INLINE

#endif

#endif //DB_EX1_ATTRIBUTES_H
