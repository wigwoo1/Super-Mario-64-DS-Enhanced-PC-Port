//cpp
#if defined(SM64DS_PLATFORM_PC)
#include "dScStage_c.h"

static int PcGraphCallback0(void *) { return 1; }
static int PcGraphCallback1(void *self)
{
    return static_cast<dScStage_c *>(self)->GraphCallback1();
}
static int PcGraphCallback2(void *self)
{
    return static_cast<dScStage_c *>(self)->GraphCallback2();
}
static int PcGraphCallback3(void *) { return 1; }
static void *sPcStageGraphVtable[] = {
    (void *)PcGraphCallback0,
    (void *)PcGraphCallback1,
    (void *)PcGraphCallback2,
    (void *)PcGraphCallback3,
};
#endif

extern char data_0208ee14;
extern char data_02092188;
extern char data_0209f388;
extern void *volatile data_0209f3c4;
extern char data_0209f43c;
extern "C" void *_ZN8dClipperC1Ev(void *object);
extern "C" void *_ZN8dClipperD1Ev(void *object);
extern "C" void func_020731dc(void *object, void *destructor, void *node);

extern "C" void __sinit_02074e84(void);
void __sinit_02074e84(void) {
#if defined(SM64DS_PLATFORM_PC)
    data_0209f3c4 = sPcStageGraphVtable;
#else
    data_0209f3c4 = &data_0208ee14;
    data_0209f3c4 = &data_02092188;
#endif
    _ZN8dClipperC1Ev(&data_0209f43c);
    func_020731dc(&data_0209f43c, (void *)_ZN8dClipperD1Ev, &data_0209f388);
}
