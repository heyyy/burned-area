#ifndef _BOOL_H
#define _BOOL_H

#ifdef true
#undef true
#endif

#ifdef false
#undef false
#endif

typedef enum {false = 0, true = 1} bool;

#endif
