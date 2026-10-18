#include "ModelBase.h"
#include <cstdlib>
#include <cstdio>

static void *released;
static unsigned releases;
extern "C" void Deallocate(void *ptr)
{
    released = ptr;
    ++releases;
    std::free(ptr);
}

// Native slot declarations are required for the real constructor's vtable.
void ModelBase::Destructor1() { this->~ModelBase(); }
void ModelBase::Destructor0() { this->~ModelBase(); }
struct CleanupModel : ModelBase {
    int DoSetFile(char *, int, int) override { return 0; }
};

int main()
{
    void *file = std::malloc(32);
    if (!file) return 2;
    {
        CleanupModel model;
        model.modelFile = static_cast<BMD_File *>(file);
    }
    {
        CleanupModel emptyModel;
    }
    if (releases != 1 || released != file) {
        std::fprintf(stderr, "FAIL: native model destruction did not release its owned file exactly once\n");
        return 1;
    }
    std::puts("PASS: recovered native model destructor releases the owned file");
}
