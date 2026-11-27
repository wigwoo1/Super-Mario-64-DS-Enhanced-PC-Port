"""Adapt the recovered Spindrift to native actor and resource layouts."""


def transform(text):
    for wrapper in ('HuwaModelFilePtr', 'HuwaAnimationFileHandle'):
        anchor = f'{wrapper}(unsigned int fileID) {{ pc_sharedfile_handle_construct(this, fileID); }}'
        assert text.count(anchor) == 1
        text = text.replace(anchor, f'{wrapper}(unsigned int fileID) : id(fileID), ptr(nullptr) {{}}')
        anchor = f'~{wrapper}() {{ pc_sharedfile_handle_destroy(this); }}'
        assert text.count(anchor) == 1
        text = text.replace(anchor, f'~{wrapper}() {{}}')

    anchor = 'int daHuwa_c::InitResources()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + """
    static const bool handlesReady = [] {
        data_ov081_02128d60.Construct(813);
        data_ov081_02128d68.Construct(814);
        return true;
    }();
    (void)handlesReady;""")

    player_position = """            int *src = (int *)((int)player + 0x5c);
            int playerPos[3];
            playerPos[0] = src[0];
            playerPos[1] = src[1];
            playerPos[2] = src[2];"""
    assert text.count(player_position) == 1
    text = text.replace(player_position, '            const Vector3 playerPos{player->mPosX, player->mPosY, player->mPosZ};')
    text = text.replace('(const Vector3 *)playerPos', '&playerPos')

    anchor = 'int daHuwa_c::Render()\n{'
    assert text.count(anchor) == 1
    text = text.replace(anchor, anchor + """
    pc_render_probe::Scope probe(this, 0, &mModelAnim.mat4x3);""")
    return '#include "hal/actor_render_probe.h"\n' + text
