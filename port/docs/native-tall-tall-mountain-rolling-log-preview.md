# Tall Tall Mountain rolling log preview

The native port registers Tall Tall Mountain’s rolling log using the same movement and collision code as the Lethal Lava Land log, with the mountain log’s own resources and travel limit.

The native actor-data audit passed. A muted replay on a fresh ROM-free kit reconstruction loaded an earned 38-Star save, entered Mission 1 through the original painting and mission selector, and confirmed that the log spawned.

Separate muted development replays used controller inputs to climb from the course entrance and walk onto the log. The collision trace confirmed that the moving log supported Mario while he walked its length. The jump in that replay missed the far bank.

Another muted replay moved the log through normal play, used the original Exit Course menu, and re-entered Mission 1 through the castle painting. A fresh log spawned at the original position, Mario regained control, and both isolated save files stayed byte-identical.

A later muted controller replay entered Mission 2, Mystery of the Monkey Cage, through the original painting and mission selector. The moving log supported Mario, and he jumped to the far bank and reached the summit. He grabbed Ukiki, followed him to the cage, and collected the Star dropped near the waterfall. The original Save & Continue advanced the isolated File A from 39 to 40 Stars and returned control in the castle. A fresh process then loaded that save, selected Mission 2 again and repeated the full route with controller inputs. Picking up the released Star kept the total at 40. Save & Continue returned Mario to the castle, and both isolated save files matched their pre-replay bytes.
