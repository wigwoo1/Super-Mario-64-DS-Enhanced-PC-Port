extern short data_020a1164[];
#if defined(SM64DS_PLATFORM_PC)
static short pc_camera_movement_yaw_offset;
static int pc_camera_movement_yaw_active;
void pc_camera_set_movement_yaw_offset(short offset, int active) {
    pc_camera_movement_yaw_offset = offset;
    pc_camera_movement_yaw_active = active;
}
#endif
short GetAngleToCamera(int i) {
    const short angle = *(short*)((char*)data_020a1164 + i * 0x24);
#if defined(SM64DS_PLATFORM_PC)
    if (i == 0 && pc_camera_movement_yaw_active)
        return (short)(angle + pc_camera_movement_yaw_offset);
#endif
    return angle;
}
