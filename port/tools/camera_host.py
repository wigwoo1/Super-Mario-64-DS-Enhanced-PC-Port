"""Recover the camera's startup records and preserve their native layout."""
import re
import struct

MODE_START, MODE_END = 0x02086fcc, 0x0208751c


def bindings(root):
    initializer = (root / 'src/unnamed/arm9/0207/__sinit_02073a24.c').read_text()
    symbols = (root / 'config/arm9/symbols.txt').read_text()
    methods = {int(address, 16): name for name, address in re.findall(
        r'^(\S+) kind:function\([^\n]*?addr:0x([\da-fA-F]+)', symbols, re.M)}
    arm = (root / 'extracted/arm9_dec.bin').read_bytes()
    pairs = {}
    for record, slot, constant in re.findall(
            r'(data_[\da-f]+)\.p([01]) = (data_[\da-f]+);', initializer):
        target, adjustment = struct.unpack_from('<II', arm, int(constant[5:], 16) - 0x02004000)
        if adjustment != 0 or target not in methods:
            raise ValueError(f'Unsupported camera descriptor {constant}')
        pairs.setdefault(record, {})[int(slot)] = methods[target]
    if len(pairs) != 19 or any(set(slots) != {0, 1} for slots in pairs.values()):
        raise ValueError('Camera startup record roster changed')
    return pairs


def owned_names(root):
    symbols = (root / 'config/arm9/symbols.txt').read_text()
    modes = {name: int(address, 16) for name, address in re.findall(
        r'^(data_[\da-f]+) kind:data\([^\n]*?addr:0x([\da-fA-F]+)', symbols, re.M)
        if MODE_START <= int(address, 16) < MODE_END}
    return bindings(root), modes


def write_sources(root, directory):
    pairs, modes = owned_names(root)
    # Three recovered workers have void return types. Their original callers
    # ignore the result, but typed wrappers avoid calling them as int functions.
    void_workers = {'func_0200987c', 'func_020094c0', 'func_02005000'}
    cpp = ['#include <cstddef>', '#include "dCamera_c.h"', 'extern "C" {']
    for name in sorted(void_workers):
        cpp += [f'void {name}(void *);',
                f'int pc_camera_{name}(void *self) {{ {name}(self); return 1; }}']
    cpp += [f'extern unsigned char {name}[];' for name in pairs]
    cpp += ['int pc_camera_state_id(const void *object) {',
            '    if (!object) return -1;',
            '    const auto *camera = static_cast<const dCamera_c *>(object);']
    for name in pairs:
        cpp.append(f'    if ((const void *)camera->mState == {name}) return 0x{name[5:]}u;')
    cpp += ['    return -2;', '}', '}',
            'static_assert(sizeof(dCamera_c) == 0x1a8, "native camera layout");',
            'static_assert(offsetof(dCamera_c, mState) == 0x138, "native camera state");',
            'static_assert(offsetof(dCamera_c, mFlags) == 0x154, "native camera flags");']
    asm = ['; Generated from local camera startup descriptors and mode bytes.',
           '.386', '.model flat', '.data', 'ALIGN 4']
    targets = {name for slots in pairs.values() for name in slots.values()}
    for name in sorted(targets):
        target = 'pc_camera_' + name if name in void_workers else name
        asm.append(f'EXTERN _{target}:PROC')
    for record, slots in pairs.items():
        asm += [f'PUBLIC _{record}', f'_{record} LABEL BYTE']
        for slot in range(2):
            name = slots[slot]
            target = 'pc_camera_' + name if name in void_workers else name
            asm += [f'DWORD OFFSET _{target}', 'DWORD 0']
    # Several state initializers index this table by index*0x28, and the
    # workers compare mode addresses. Interior labels must share one allocation.
    arm = (root / 'extracted/arm9_dec.bin').read_bytes()
    asm.append('ALIGN 4')
    for address in range(MODE_START, MODE_END, 4):
        for name, label in modes.items():
            if label == address:
                asm += [f'PUBLIC _{name}', f'_{name} LABEL BYTE']
        value = struct.unpack_from('<I', arm, address - 0x02004000)[0]
        asm.append(f'DWORD 0{value:08x}h')
    asm.append('END')
    directory.mkdir(parents=True, exist_ok=True)
    outputs = []
    for name, lines in (('camera_native_data.cpp', cpp), ('camera_native_data.asm', asm)):
        output = directory / name
        text = '\n'.join(lines) + '\n'
        if not output.exists() or output.read_text() != text:
            output.write_text(text)
        outputs.append(output.as_posix())
    return outputs


def transform(text):
    render='int dCamera_c::Render()\n{'
    assert text.count(render)==1,'camera render changed'
    text=text.replace(render,render+'\n    ntr::video_set_cannon_sight(false);',1)
    sight='_ZN3OAM6RenderEbP7OamAttriiii5Fix12IiES3_ii(0, (OamAttr *)&data_ov002_0210c3b0, 0x80, 0x60, -1, -1, 0x1000, 0x1000, 0, -1);'
    assert text.count(sight)==1,'original cannon sight submission changed'
    text=text.replace(sight,'ntr::video_set_cannon_sight(true);',1)
    old = 'typedef int (CamPmfHost::*CamPmf)();'
    new = ('struct CamPmf { int (__cdecl *function)(void *); int adjustment; };\n'
           'static_assert(sizeof(CamPmf) == 8, "camera callback stride");')
    if old not in text:
        raise ValueError('Camera dispatcher declaration changed')
    text = text.replace(old, new)
    for old, new in (
            ('return (c->**p)();', 'return p->function(c_);'),
            ('r5 = (ch->**pp)();', 'r5 = pp->function(self);')):
        if text.count(old) != 1:
            raise ValueError('Camera dispatcher body changed')
        text = text.replace(old, new)
    anchor = '    void *lookdir = &data_02086efc;'
    if text.count(anchor) != 1:
        raise ValueError('Camera render eye conversion changed')
    # Scale the temporary render eye, before the original /8 conversion. The
    # tracking camera and saved dialogue/cutscene coordinates remain untouched.
    anchor = '    {\n        s32 s2 = (s32)(sp[2] + 4) >> 3;'
    if text.count(anchor) != 1:
        raise ValueError('Camera render coordinate conversion changed')
    text = text.replace(anchor, '    pc_camera_zoom(this, sp);\n\n' + anchor)
    text = text.replace('// @symbol _ZN9dCamera_c6RenderEv',
                        'static bool pc_camera_gameplay(const dCamera_c *);\n'
                        'static void pc_camera_zoom(dCamera_c *, s32 *);\n\n// @symbol _ZN9dCamera_c6RenderEv')
    # The original lens eases from its authored angle to a narrower idle lens.
    # Keeping the eye distance steady alone cannot remove that visible zoom.
    # Use the authored lens while the player controls the ordinary camera;
    # scripted and inspection cameras keep the original blend.
    lens = 's32 t = *(s32 *)((char *)self + 0x168);'
    if text.count(lens) != 1:
        raise ValueError('Camera movement lens blend changed')
    text = text.replace(lens,
        's32 t = pc_camera_gameplay(self) ? 0x1000 : *(s32 *)((char *)self + 0x168);')
    text = ('#include "ntr/video.h"\n#include "ntr/input.h"\n#include "ntr/rt.h"\n#include "ntr/camera_controls.h"\n'
            '#include <algorithm>\n#include <cmath>\n#include <climits>\n' + text)
    text += r'''
extern "C" int pc_chomp_release_target(int *);
extern "C" void pc_camera_set_movement_yaw_offset(short, int);

static bool pc_camera_gameplay(const dCamera_c *camera)
{
    const auto *player=reinterpret_cast<const unsigned char *>(camera->mTargetPlayer);
    return player && *reinterpret_cast<const u16 *>(player+0xc)==0xbf &&
        !player[0x6f6] && !player[0x709] &&
        !data_0209fc48 && !data_0209f294 && !data_0209f20c && !data_0209f2c4 &&
        camera->mState!=reinterpret_cast<const dCamera_c::State *>(&data_0209b098);
}

// Native distance zoom, using the camera's own background collision filter.
static void pc_camera_zoom(dCamera_c *camera, s32 *view)
{
    static double orbit_yaw = 0.0, orbit_pitch = 0.0;
    static bool orbit_active = false, orbit_initialized = false;
    static ntr::FreeCameraResponse response;
    static unsigned long long control_tick=~0ull;
    // Player.h pins actorID at 0xc and control-disabled/no-control bytes at
    // 0x6f6/0x709. Avoid importing its collision classes into this C shard.
    auto *player = reinterpret_cast<unsigned char *>(camera->mTargetPlayer);
    const bool is_player = player && *reinterpret_cast<const u16 *>(player+0xc) == 0xbf;
    const bool allowed = is_player &&
        !player[0x6f6] && !player[0x709] &&
        !data_0209fc48 && !data_0209f294 && !data_0209f20c && !data_0209f2c4;
    int chomp[3];
    if(is_player && (player[0x6f6] || player[0x709]) &&
       !data_0209f2c4 && pc_chomp_release_target(chomp)) {
        // Retail actor tracking keeps the eye fixed and skips its normal
        // distance/collision branch (02009e70). Keep the original look target
        // but frame the moving head outside its conservative 350-unit shadow
        // footprint. This only adjusts the temporary render eye, not gameplay.
        const double dx=double(view[3])-view[0],dy=double(view[4])-view[1],dz=double(view[5])-view[2];
        const double distance=std::sqrt(dx*dx+dy*dy+dz*dz);
        const double half_angle=*reinterpret_cast<const short *>(reinterpret_cast<const char *>(camera)+0x17a)*6.283185307179586/65536.0;
        const double minimum=350.0*4096/std::sin(half_angle>0.1?half_angle:0.1);
        if(distance>1 && distance<minimum) {
            Vec3 start={view[3],view[4],view[5]};
            Vec3 end={view[0]+static_cast<int>(dx*minimum/distance),
                      view[1]+static_cast<int>(dy*minimum/distance),
                      view[2]+static_cast<int>(dz*minimum/distance)};
            dBgCh_Lin line;
            _ZN9dBgCh_LinC1Ev(&line);
            func_0200897c(camera,&line);
            _ZN9dBgCh_Lin13SetObjAndLineERK7Vector3S2_P8dActor_c(&line,&start,&end,nullptr);
            const bool blocked=_ZN9dBgCh_Lin10DetectClsnEv(&line)!=0;
            if(blocked) {
                Vec3 hit;_ZN9dBgCh_Lin10GetClsnPosEv(&hit,&line);
                const double hx=double(hit.x)-start.x,hy=double(hit.y)-start.y,hz=double(hit.z)-start.z;
                const double length=std::sqrt(hx*hx+hy*hy+hz*hz);
                const double fraction=length>32*4096?(length-32*4096)/length:0;
                end.x=start.x+static_cast<int>(hx*fraction);
                end.y=start.y+static_cast<int>(hy*fraction);
                end.z=start.z+static_cast<int>(hz*fraction);
            }
            _ZN9dBgCh_LinD1Ev(&line);
            view[3]=end.x;view[4]=end.y;view[5]=end.z;
            if(std::getenv("SM64DS_PC_CAMERA_ZOOM_TRACE"))
                std::fprintf(stderr,"[chomp camera] tick=%llu blocked=%d base=%d,%d,%d eye=%d,%d,%d target=%d,%d,%d chomp=%d,%d,%d\n",
                    static_cast<unsigned long long>(ntr::rt_frame()),int(blocked),start.x,start.y,start.z,
                    end.x,end.y,end.z,view[0],view[1],view[2],chomp[0],chomp[1],chomp[2]);
        }
    }
    static dCamera_c *previous_camera=nullptr;
    static bool recenter_from_no_control=false;
    if(previous_camera!=camera) {previous_camera=camera;recenter_from_no_control=false;}
    const auto recenter=reinterpret_cast<dCamera_c::State *>(&data_0209b0e8);
    if(camera->mState!=recenter) recenter_from_no_control=false;
    else if(is_player && (player[0x6f6] || player[0x709])) recenter_from_no_control=true;
    if(allowed && recenter_from_no_control) {
        // GoBehindPlayer's door-exit state continually targets 128 units.
        // Keep it during no-control, then restore tracking when play resumes.
        camera->ChangeState(reinterpret_cast<dCamera_c::State *>(&data_0209b008));
        recenter_from_no_control=false;
        view[0]=camera->lookAt.x;view[1]=camera->lookAt.y;view[2]=camera->lookAt.z;
        view[3]=camera->pos.x;view[4]=camera->pos.y;view[5]=camera->pos.z;
        std::fprintf(stderr,"[camera zoom] recenter-exit tick=%llu\n",static_cast<unsigned long long>(ntr::rt_frame()));
    }
    auto settings = ntr::video_get_settings();
    static unsigned long long last_trace=0;
    if(std::getenv("SM64DS_PC_CAMERA_ZOOM_TRACE") && ntr::rt_frame()>=last_trace+30) {
        last_trace=ntr::rt_frame();
        const auto *bytes=reinterpret_cast<const unsigned char *>(camera);
        std::fprintf(stderr,"[camera native] tick=%llu state=%p mode_offset=%d flags=%x allowed=%d scale=%d,%d view=%d,%d,%d,%d,%d,%d\n",
            static_cast<unsigned long long>(ntr::rt_frame()),static_cast<void *>(camera->mState),
            camera->mState_13c-reinterpret_cast<int>(&data_02086fcc),camera->mFlags,int(allowed),
            *reinterpret_cast<const int *>(bytes+0x104),*reinterpret_cast<const int *>(bytes+0x108),
            view[0],view[1],view[2],view[3],view[4],view[5]);
        const auto controls=ntr::input_state();
        std::fprintf(stderr,"[camera controls] tick=%llu actor=%u disabled=%u no_control=%u locks=%d,%u,%u,%u fov=%d blend=%d mode=%u pad=%u right=%d,%d\n",
            static_cast<unsigned long long>(ntr::rt_frame()),player?unsigned(*reinterpret_cast<const u16 *>(player+0xc)):0,
            is_player?unsigned(player[0x6f6]):0,is_player?unsigned(player[0x709]):0,
            data_0209fc48,unsigned(data_0209f294),unsigned(data_0209f20c),unsigned(data_0209f2c4),
            int(*reinterpret_cast<const short *>(bytes+0x17a)),*reinterpret_cast<const int *>(bytes+0x168),
            unsigned(settings.camera_mode),unsigned(controls.buttons),int(controls.right_x),int(controls.right_y));
    }
    static unsigned long long handled = ~0ull;
    if (ntr::input_take_camera_zoom() && handled != ntr::rt_frame()) {
        handled = ntr::rt_frame();
        if (allowed) {
            // X's original cycle can enter St_CameraZoom (camera state b098).
            // Use its normal flag-driven exit so the player state cleans up
            // on the following tick; restore the normal camera immediately.
            if(camera->mState == reinterpret_cast<dCamera_c::State *>(&data_0209b098) || camera->mState==recenter) {
                *reinterpret_cast<u16 *>(player+0x6ce) &= ~4u;
                player[0x715]=0;
                camera->ChangeState(reinterpret_cast<dCamera_c::State *>(&data_0209b008));
                view[0]=camera->lookAt.x;view[1]=camera->lookAt.y;view[2]=camera->lookAt.z;
                view[3]=camera->pos.x;view[4]=camera->pos.y;view[5]=camera->pos.z;
                std::fprintf(stderr,"[camera zoom] inspection-exit tick=%llu\n",handled);
            }
            settings.camera_zoom = static_cast<unsigned char>((settings.camera_zoom+1)%4);
            ntr::video_set_settings(settings);
            // Original camera-mode cues: outward zoom / return to normal.
            func_02012790(settings.camera_zoom ? 7u : 6u);
            const bool saved = ntr::video_save_settings();
            std::fprintf(stderr,"[camera zoom] tick=%llu option=%u saved=%d\n",
                handled,unsigned(settings.camera_zoom),int(saved));
        }
    }
    static unsigned long long mode_handled=~0ull;
    if(ntr::input_take_camera_mode() && mode_handled!=ntr::rt_frame()) {
        mode_handled=ntr::rt_frame();
        if(allowed) {
            settings.camera_mode^=1;
            ntr::video_set_settings(settings);
            const bool saved=ntr::video_save_settings();
            // Keep the familiar camera-mode cue when switching to or from free orbit.
            func_02012790(settings.camera_mode ? 7u : 6u);
            orbit_active=false;orbit_initialized=false;
            std::fprintf(stderr,"[camera mode] tick=%llu mode=%s saved=%d\n",
                mode_handled,settings.camera_mode?"free":"preset",int(saved));
        }
    }
    if (!allowed) { pc_camera_set_movement_yaw_offset(0,0); orbit_active=false; orbit_initialized=false; response.reset(); control_tick=~0ull; return; }
    const auto input = ntr::input_state();
    if(settings.camera_mode!=1) { pc_camera_set_movement_yaw_offset(0,0); orbit_active=false; orbit_initialized=false; response.reset(); control_tick=~0ull; }
    // Free camera uses the camera object's stable tracked pose as its anchor.
    // Never derive distance from the previous render-only view: that view was
    // already orbited/zoomed and would compound every render pass.
    const Vec3 target={camera->lookAt.x,camera->lookAt.y,camera->lookAt.z};
    const Vec3 native_eye={camera->pos.x,camera->pos.y,camera->pos.z};
    const double native_dx=double(native_eye.x)-target.x;
    const double native_dy=double(native_eye.y)-target.y;
    const double native_dz=double(native_eye.z)-target.z;
    const double native_distance=std::sqrt(native_dx*native_dx+native_dy*native_dy+native_dz*native_dz);
    // Keep the farthest normal gameplay distance seen in this camera session.
    // Retail pulls the camera inward when Yoshi stops, which reads as a zoom.
    // Collision checks below can still shorten this distance when obstructed.
    static dCamera_c *distance_camera=nullptr;
    static double distance_floor=0.0;
    if(distance_camera!=camera) { distance_camera=camera; distance_floor=0.0; }
    const double max_gameplay_distance=900.0*4096.0;
    distance_floor=std::max(distance_floor,std::min(native_distance,max_gameplay_distance));
    const auto current_yaw = std::atan2(native_dz,native_dx);
    if (!orbit_initialized) {
        orbit_yaw=current_yaw;
        orbit_pitch=std::asin(std::clamp(native_dy/std::max(1.0,native_distance),-1.0,1.0));
        orbit_initialized=true;
    }
    if(settings.camera_mode==1) {
        const auto tick=ntr::rt_frame();
        const double elapsed=control_tick==~0ull || tick<control_tick?1.0/30.0:double(tick-control_tick)/60.0;
        control_tick=tick;
        const auto turn=response.update(input.right_x,input.right_y,settings,elapsed);
        orbit_yaw += turn.yaw;
        orbit_pitch += turn.pitch;
        orbit_pitch=std::clamp(orbit_pitch,-1.15,1.15);
        orbit_active=turn.yaw!=0 || turn.pitch!=0;
        if(std::getenv("SM64DS_PC_CAMERA_ORBIT_TRACE") && tick%30==0)
            std::fprintf(stderr,"[camera response] tick=%llu elapsed=%.6f turn=%.8f,%.8f sensitivity=%u invert=%u,%u acceleration=%u\n",
                static_cast<unsigned long long>(tick),elapsed,turn.yaw,turn.pitch,unsigned(settings.camera_sensitivity),
                unsigned(settings.camera_invert_x),unsigned(settings.camera_invert_y),unsigned(settings.camera_acceleration));
    }
    if(settings.camera_mode==1) {
        // Match the game's own horizontal-angle convention (atan2(x,z)),
        // which is the opposite orientation of the render-space atan2(z,x).
        const int free_camera_angle=static_cast<int>(std::lround(
            (1.5707963267948966-orbit_yaw)*65536.0/6.283185307179586));
        const int native_camera_angle=static_cast<int>(camera->mAngleY)+
            *reinterpret_cast<const short *>(reinterpret_cast<const char *>(camera)+0x188);
        const int movement_offset=((free_camera_angle-native_camera_angle+32768)&0xffff)-32768;
        pc_camera_set_movement_yaw_offset(static_cast<short>(std::clamp(movement_offset,-32768,32767)),1);
        const int denominator[]={4,3,2,1};
        const double distance=settings.camera_zoom>0 && settings.camera_zoom<4
            ? distance_floor*4.0/denominator[settings.camera_zoom] : distance_floor;
        Vec3 start=native_eye;
        Vec3 desired={target.x+static_cast<int>(std::cos(orbit_pitch)*std::cos(orbit_yaw)*distance),
                      target.y+static_cast<int>(std::sin(orbit_pitch)*distance),
                      target.z+static_cast<int>(std::cos(orbit_pitch)*std::sin(orbit_yaw)*distance)};
        dBgCh_Lin line;
        _ZN9dBgCh_LinC1Ev(&line);func_0200897c(camera,&line);
        _ZN9dBgCh_Lin13SetObjAndLineERK7Vector3S2_P8dActor_c(&line,&start,&desired,nullptr);
        const bool blocked=_ZN9dBgCh_Lin10DetectClsnEv(&line)!=0;
        if(blocked) {
            Vec3 hit;_ZN9dBgCh_Lin10GetClsnPosEv(&hit,&line);
            const double dx=double(hit.x)-start.x,dy=double(hit.y)-start.y,dz=double(hit.z)-start.z;
            const double length=std::sqrt(dx*dx+dy*dy+dz*dz);
            const double fraction=length>32*4096?(length-32*4096)/length:0;
            desired={start.x+static_cast<int>(dx*fraction),start.y+static_cast<int>(dy*fraction),start.z+static_cast<int>(dz*fraction)};
        }
        _ZN9dBgCh_LinD1Ev(&line);
        view[0]=target.x;view[1]=target.y;view[2]=target.z;
        view[3]=desired.x;view[4]=desired.y;view[5]=desired.z;
        if(std::getenv("SM64DS_PC_CAMERA_ORBIT_TRACE")) {
            static long long last_x=0,last_z=0;static unsigned long long last=0;
            const long long dx=static_cast<long long>(view[3])-view[0],dz=static_cast<long long>(view[5])-view[2];
            if(last==0 || ntr::rt_frame()>=last+30) {
                std::fprintf(stderr,"[camera orbit] tick=%llu stick=%d,%d distance=%.2f angle=%lld,%lld changed=%d moveYaw=%d cameraAngle=%d nativeAngle=%d\n",
                    static_cast<unsigned long long>(ntr::rt_frame()),int(input.right_x),int(input.right_y),
                    std::sqrt(double(view[3]-view[0])*(view[3]-view[0])+double(view[4]-view[1])*(view[4]-view[1])+double(view[5]-view[2])*(view[5]-view[2]))/4096.0,
                    dx,dz,int(dx!=last_x || dz!=last_z),movement_offset,free_camera_angle,native_camera_angle);
                last_x=dx;last_z=dz;last=ntr::rt_frame();
            }
        }
        return;
    }
    if (settings.camera_zoom>3) return;
    const int denominator[] = {4,3,2,1};
    Vec3 start = {view[3],view[4],view[5]}, end;
    const double dx=double(start.x)-view[0],dy=double(start.y)-view[1],dz=double(start.z)-view[2];
    const double current_distance=std::sqrt(dx*dx+dy*dy+dz*dz);
    const double distance=distance_floor*4.0/denominator[settings.camera_zoom];
    if(current_distance<1 || distance<1) return;
    end={view[0]+static_cast<int>(dx*distance/current_distance),
         view[1]+static_cast<int>(dy*distance/current_distance),
         view[2]+static_cast<int>(dz*distance/current_distance)};
    bool blocked=false;
    if(distance>current_distance) {
        // Extend the engine's collision-adjusted eye ray and retain a small
        // clearance from any wall or camera-solid surface.
        dBgCh_Lin line;
        _ZN9dBgCh_LinC1Ev(&line);
        func_0200897c(camera,&line);
        _ZN9dBgCh_Lin13SetObjAndLineERK7Vector3S2_P8dActor_c(&line,&start,&end,nullptr);
        blocked=_ZN9dBgCh_Lin10DetectClsnEv(&line)!=0;
        if (blocked) {
            Vec3 hit;
            _ZN9dBgCh_Lin10GetClsnPosEv(&hit,&line);
            const double hx=double(hit.x)-start.x,hy=double(hit.y)-start.y,hz=double(hit.z)-start.z;
            const double length=std::sqrt(hx*hx+hy*hy+hz*hz);
            const double fraction=length>32*4096 ? (length-32*4096)/length : 0;
            end={start.x+static_cast<int>(hx*fraction),
                 start.y+static_cast<int>(hy*fraction),
                 start.z+static_cast<int>(hz*fraction)};
        }
        _ZN9dBgCh_LinD1Ev(&line);
    }
    view[3]=end.x;view[4]=end.y;view[5]=end.z;
    if (std::getenv("SM64DS_PC_CAMERA_ZOOM_TRACE") && ntr::rt_frame()%30==0)
        std::fprintf(stderr,"[camera zoom view] tick=%llu option=%u blocked=%d base=%d,%d,%d eye=%d,%d,%d target=%d,%d,%d\n",
            static_cast<unsigned long long>(ntr::rt_frame()),unsigned(settings.camera_zoom),int(blocked),
            start.x,start.y,start.z,end.x,end.y,end.z,view[0],view[1],view[2]);
}

extern "C" void pc_camera_restore_after_inspection(void *object)
{
    auto *camera=static_cast<dCamera_c *>(object);
    if(camera && camera->mState == reinterpret_cast<dCamera_c::State *>(&data_0209b098))
        camera->ChangeState(reinterpret_cast<dCamera_c::State *>(&data_0209b008));
}
'''
    return text
