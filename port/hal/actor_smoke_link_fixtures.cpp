#include <cstdio>
#include <cstdlib>

/*
 * The actor lifecycle fixture links a broad set of recovered translation
 * units, including scene-transition helpers that are not part of its test.
 * Keep an explicit failure if one of those unrelated helpers is reached.
 */
extern "C" int func_02043098(int, int, int, int)
{
    std::fputs("FATAL: actor smoke reached scene transition helper func_02043098\n", stderr);
    std::abort();
}
