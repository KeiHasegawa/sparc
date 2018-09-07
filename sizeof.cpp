#include "stdafx.h"

#ifdef CXX_GENERATOR
#include "cxx_core.h"
#else  // CXX_GENERATOR
#include "c_core.h"
#endif  // CXX_GENERATOR

#include "sparc.h"

extern "C" DLL_EXPORT int generator_sizeof(int n)
{
  using namespace COMPILER;
  switch ((type::id)n) {
  case type::SHORT:
    return 2;
  case type::INT:
    return 4;
  case type::LONG:
    return 4;
  case type::LONGLONG:
    return 8;
  case type::FLOAT:
    return 4;
  case type::DOUBLE:
    return 8;
  case type::LONG_DOUBLE:
    return 16;
  default:
    assert((type::id)n == type::POINTER);
    return 4;
  }
}
