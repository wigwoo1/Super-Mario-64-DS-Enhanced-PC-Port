//cpp
// @symbol _ZN7PathPtr6FromIDEj
// @symbol _ZN7PathPtr12SetPathTableEP7PathDefi
// @symbol _ZN7PathPtr12GetNodeTableEv
// @symbol _ZN7PathPtr12SetNodeTableEPv
// @symbol _ZNK7PathPtr7GetNodeER7Vector3j
// @symbol _ZNK7PathPtr5LoopsEv
// @symbol _ZNK7PathPtr13GetPullFactorEv
// @symbol _ZNK7PathPtr9GetUnk004Ev
// @symbol _ZNK7PathPtr8NumNodesEv
// @symbol _ZN7PathPtr6SetDefEP7PathDef
// @symbol _ZN7PathPtrC1Ev
/* PathPtr -- a read cursor over one entry of the level's path table.
 *
 * FromID binds the handle to the id'th PathDef; the accessors read through
 * def, and GetNode walks the shared flat table of 6-byte (three s16) node
 * records that def->firstNode indexes into. The table helpers are the
 * class's own plumbing: LoadPathObjects hands it the definition table and
 * LoadPathNodeObjects the node table (src/stage/LevelObjects.cpp).
 *
 * mwccarm emits .text in reverse source order, so the definitions below run
 * ROM-descending; the roster above reads ROM-ascending. */
#include "PathPtr.h"

/* Shared per-level tables. These are ARM9 BSS globals on the console and are
   populated as level object files load; keep the same zero-initialized state
   in the native process. */
PathDef *PATH_DEFS = 0;
char *PATH_NODES = 0;
int NUM_PATHS = 0;

PathPtr::PathPtr() : def(0), unk_004(0) {}

void PathPtr::SetDef(PathDef *d)
{
    def = d;
}

unsigned int PathPtr::NumNodes() const
{
    return def->numNodes;
}

unsigned int PathPtr::GetUnk004() const
{
    return def->unk_004;
}

unsigned int PathPtr::GetPullFactor() const
{
    return def->flags & 0x7f;
}

unsigned int PathPtr::Loops() const
{
    return def->flags & 0x80;
}

void PathPtr::GetNode(Vector3 &node, unsigned int idx) const
{
    char *base = GetNodeTable();
    u16 first = def->firstNode;
    char *row = base + first * 6;
    int off = idx * 6;
    node.x = ((int)*(s16 *)(row + off)) << 12;
    node.y = ((int)*(s16 *)(row + off + 2)) << 12;
    node.z = ((int)*(s16 *)(row + off + 4)) << 12;
}

void PathPtr::SetNodeTable(void *nodes)
{
    PATH_NODES = (char *)nodes;
}

char *PathPtr::GetNodeTable()
{
    return PATH_NODES;
}

void PathPtr::SetPathTable(PathDef *defs, int numPaths)
{
    PATH_DEFS = defs;
    NUM_PATHS = numPaths;
}

void PathPtr::FromID(unsigned int id)
{
    SetDef(PATH_DEFS + id);
}
