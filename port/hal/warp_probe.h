#pragma once
#include "dBgCh_Gnd.h"
#include "dBgCh_Lin.h"

static void pc_trace_warp_player(Player *player){
    static bool wasTeleport=false;static uint64_t last=~uint64_t(0);
    if(!std::getenv("SM64DS_PC_WARP_TRACE") || last==ntr::rt_frame())return;
    last=ntr::rt_frame();
    static int lastLevel=-1,lastCount=-1;
    int count=0;
    for(auto *actor=dActor_c::FindWithActorID(347,nullptr);actor && count<32;
        actor=dActor_c::FindWithActorID(347,actor))++count;
    if(lastLevel!=data_0209f2f8 || lastCount!=count){
        std::fprintf(stderr,"[native warp] census tick=%llu level=%d count=%d\n",
            static_cast<unsigned long long>(last),int(data_0209f2f8),count);
        lastLevel=data_0209f2f8;lastCount=count;
    }
    if(!player){wasTeleport=false;return;}
    const char *state=pc_player_state_name(player);
    const bool teleport=!std::strcmp(state,"St_Teleport_Main");
    if(teleport || wasTeleport)std::fprintf(stderr,
        "[native warp] player tick=%llu state=%s step=%u opacity=%u destination=%u cooldown=%u pos=%d,%d,%d control=%u,%u\n",
        static_cast<unsigned long long>(last),state,player->mStateStep,player->mOpacity,
        player->mTeleportId,player->mTeleportTimer,player->mPosX,player->mPosY,player->mPosZ,
        player->mIsControlDisabled,player->mIsNoControl);
    wasTeleport=teleport;
}

// Opt-in QA observation only: production ground queries, no actor placement.
static void pc_trace_warp_ground(Player *player) {
    static bool done=false;
    const char *path=std::getenv("SM64DS_PC_WARP_GROUND_CAPTURE");
    const char *automation=std::getenv("SM64DS_PC_AUTOMATION_ONLY");
    if(done || !path || !automation || std::strcmp(automation,"1") ||
       ntr::rt_frame()<1700 || data_0209f2f8!=6 || !player || player->aliveState!=1)return;
    done=true;
    FILE *file=std::fopen(path,"w");
    if(!file)return;
    std::fputs("x,z,hit,height\n",file);
    for(int z=3800;z<=6600;z+=50)for(int x=400;x<=2800;x+=50){
        dBgCh_Gnd query;
        const Vector3 position={x*4096,1500*4096,z*4096};
        query.SetObjAndPos(position,player);
        const int hit=query.DetectClsn();
        std::fprintf(file,"%d,%d,%d,%d\n",x,z,hit,hit?query.clsnY:0);
    }
    std::fclose(file);
    if(const char *segments=std::getenv("SM64DS_PC_WARP_LINE_SEGMENTS")){
        char lines[1024];std::snprintf(lines,sizeof(lines),"%s.lines.csv",path);
        if(FILE *out=std::fopen(lines,"w")){
            std::fputs("ax,az,bx,bz,offset,height,hit,x,y,z\n",out);
            while(segments && *segments){
                int ax,az,bx,bz;
                if(std::sscanf(segments,"%d:%d:%d:%d",&ax,&az,&bx,&bz)!=4)break;
                if(std::abs(ax)>32000 || std::abs(az)>32000 || std::abs(bx)>32000 || std::abs(bz)>32000)break;
                int floor=0;
                for(const Vector3 point:{Vector3{ax*4096,1500*4096,az*4096},Vector3{bx*4096,1500*4096,bz*4096}}){
                    dBgCh_Gnd ground;ground.SetObjAndPos(point,player);
                    if(ground.DetectClsn() && ground.clsnY>floor)floor=ground.clsnY;
                }
                const double dx=bx-ax,dz=bz-az,length=std::hypot(dx,dz);
                if(length>0)for(int offset:{-100,0,100})for(int height:{40,120}){
                    const int ox=static_cast<int>(-dz/length*offset*4096),oz=static_cast<int>(dx/length*offset*4096);
                    const Vector3 a={ax*4096+ox,floor+height*4096,az*4096+oz};
                    const Vector3 b={bx*4096+ox,floor+height*4096,bz*4096+oz};
                    dBgCh_Lin line;line.SetObjAndLine(a,b,player);
                    const int hit=line.DetectClsn();
                    std::fprintf(out,"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",ax,az,bx,bz,offset,height,hit,line.lineEnd.x,line.lineEnd.y,line.lineEnd.z);
                }
                const char *next=std::strchr(segments,',');segments=next?next+1:nullptr;
            }
            std::fclose(out);
        }
    }
    std::fprintf(stderr,"[native warp probe] production ground grid captured tick=%llu; no player travel\n",
        static_cast<unsigned long long>(ntr::rt_frame()));
}
