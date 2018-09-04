#include "stdafx.h"

#ifdef CXX_GENERATOR
#include "cxx_core.h"
#else  // CXX_GENERATOR
#include "c_core.h"
#endif  // CXX_GENERATOR

#include "sparc.h"

extern "C" DLL_EXPORT int generator_sizeof(const COMPILER::type* T)
{
        using namespace COMPILER;
        T = T->unqualified();

#ifdef _MSC_VER
        if (typeid(*T) == typeid(long_double_type))
                return 16;
#endif  // _MSC_VER
    return T->size();
}

extern "C" DLL_EXPORT bool generator_require_align()
{
        return true;
}
