#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
extern "C" {
struct Thing {int *items;int count;int *links;int flags;void *owner;};
static alignas(4) unsigned char curve[0x30];
static int curve_values[4], curve_flag, curve_count;
void *func_ov007_020c2b9c(int,int*);
void *func_ov007_020c2c68(int type,int count,int flags) {
    if(type || count!=curve_count || flags!=curve_flag)std::exit(7);
    std::memset(curve,0xa5,sizeof(curve));
    *reinterpret_cast<int**>(curve+0x20)=curve_values;
    return curve;
}
Thing *func_ov007_020c798c(void *,int,void *,int);
static alignas(4) unsigned char arena[256];
static unsigned used,allocations,clears,items,links;
void *func_ov007_020c3df4(int heap,int bytes) {
    if(heap || bytes<0 || used+bytes+8>sizeof(arena))std::exit(2);
    auto *result=arena+used+4;used+=bytes+8;++allocations;
    return result;
}
void MultiStore_Int(int value,void *destination,int bytes) {
    // Fail before an original missing-argument call can corrupt the test stack.
    if(destination!=arena+4 || value!=0 || bytes!=20) {
        std::fprintf(stderr,"FAIL: initializer did not receive the allocated object\n");
        std::exit(1);
    }
    ++clears;std::memset(destination,0,bytes);
}
int func_ov007_020c844c(int zero,void *source) {
    if(zero || source!=reinterpret_cast<void*>(0x1234))std::exit(3);
    return 100+items++;
}
int func_ov007_020c80a4(){return 200+links++;}
}
int main() {
    int source[4]={17,-3,99,1234};
    for(curve_count=0;curve_count<=4;++curve_count)for(curve_flag=0;curve_flag<2;++curve_flag) {
        for(auto &v:curve_values)v=0x12345678;
        void *result=func_ov007_020c2b9c(curve_count,curve_flag?source:nullptr);
        if(result!=curve || *reinterpret_cast<unsigned short*>(curve+8)!=curve_count ||
           curve[7]!=0xa5 || curve[10]!=0xa5)return 8;
        for(int i=0;i<4;++i)
            if(curve_values[i]!=(curve_flag && i<curve_count?source[i]:0x12345678))return 9;
    }
    for(int count=1;count<=4;++count)for(int flags=0;flags<4;++flags) {
        std::memset(arena,0xa5,sizeof(arena));used=allocations=clears=items=links=0;
        auto *object=func_ov007_020c798c(reinterpret_cast<void*>(0x5678),count,reinterpret_cast<void*>(0x1234),flags);
        if(object!=reinterpret_cast<Thing*>(arena+4) || object->owner!=reinterpret_cast<void*>(0x5678) ||
           object->count!=count || object->flags!=flags || clears!=1 ||
           items!=((flags&1)?count:0) || links!=(count>1?count-((flags&2)?0:1):0) ||
           arena[0]!=0xa5 || arena[24]!=0xa5 || (count==1 && object->links!=nullptr))return 4;
        if(flags&1)for(int i=0;i<count;++i)if(object->items[i]!=100+i)return 5;
        for(unsigned i=0;i<links;++i)if(object->links[i]!=200+int(i))return 6;
    }
    std::puts("PASS: original menu constructor initializes its allocated object; all count/flag paths and canaries preserved");
}
