# 11_PROJECTILES_BUFFS_PARTICLES

Planned event chain:

- ability used
- projectile spawned
- projectile moves
- projectile impacts
- damage/buff/particle/sound/noise emitted
- horde attention changes
- UI/log/minimap update

## Design intent

Each transition should emit events that can be replayed and traced later for debugging.
