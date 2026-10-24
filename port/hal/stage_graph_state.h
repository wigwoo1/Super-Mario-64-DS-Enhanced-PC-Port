#ifndef PC_STAGE_GRAPH_STATE_H
#define PC_STAGE_GRAPH_STATE_H

/* ROM 0209f3c4..0209f3e8 is one graphics callback with its transform state. */
struct PcStageGraphState {
    void *vtable;
    int matrix[4];
    int center_x, center_y, destination_x, destination_y;
};
#ifdef __cplusplus
extern "C" {
#endif
extern struct PcStageGraphState data_0209f3c4;
int pc_stage_graph_apply(void *self);
int pc_stage_graph_draw_particles(void *self);
void pc_stage_graph_prepare(void);
#ifdef __cplusplus
}
static_assert(sizeof(PcStageGraphState) == 36, "graphics callback requires x86 layout");
#endif
#endif
