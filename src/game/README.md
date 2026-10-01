## Explanation on BloonID (BID)

Each Bloon has a unique BID. Each projectile has a list of BID of bloons it has hit.

BID is made of 3 parts: (u32) family, layer, (u32) tree
- Each spawned bloon has a unique `family`, so every spawned bloon "family" is unique.
- `layer` determines how far into the lineage a given bloon is. `layer` determines the bitmask on `tree`.
- `tree` represents the position in the lineage from the initial spawned bloon.

The `tree` is a binary tree, represented by `u32` where bits (from low to high) indicate going right and left in the tree.
So, `0b10` is a parent of `0b010` and `0b110` (assuming `layer` to be 2 and 3 from parent and child, respectively).

To check whether or not some Bloon is a descendant of another bloon: take smallest `layer` of both BIDs, mask both `tree`s with the smallest `layer` amount of low bits and compare for equality. If equal, the bloon with smaller `layer` is the parent of the other bloon.

Note that overflow in `family` is acceptable, since it's just a counting up unique id. By the time we get through all possible ids, first ids would be long gone.

### Alternative solution:

I thought a bit about it, and there's another possible solution to this problem. I could have every bloon get a dense id. And then have a data structure, where I store all parent-child relationships. So, for example, a map of child to all ancestors. I have a child and a parent ID. I index into the big map by child ID. There, I have another map. I index by parent ID. If it's present, I conclude that projectile cannot hit. This completely eliminates all problems with regrow bloons. Also, when I pop a bloon, I just copy their parent table for every child and that's it. Usually these tables are not going to be big at all, so it's not an issue at all. It's actually a far simpler solution than what I came up with first. I have no idea how I didn't think of this first. I mean, my complicated system does have *some* advantages. Until I get to regrow bloons, it's actually very efficient. Sure it takes a bit of extra space per bloon, but ancestry checks are just 3 integer comparisons. No memory accesses. So it *is* fast. But, regrows exist and ruin everything.

## Bloon stats (based on tier)

- Initial HP
- Initial type
- Base speed
- Children
- Hitbox
- Special stats
  - Regrows to
  - Regrow time
  - Immune to speed/position related debuffs
  - Fortified HP bonus (x2 for all except Lead)

## Bloon types

Solely determined by own tier:
- Black
- White
- Lead
- Purple
- Ceramic
- Blimp
- Boss

Inherited from parent bloons (and add extra visual effects):
- Frozen (makes slow/stop = paired with a debuff)
- Camo
- Fortified (gives more hp = on add/remove fuck around with hp values)
- Regrow (makes bloons regrow to a certain point = extra info required)

Regrow is not like the other 3. There are no abilities that specifically target regrow bloons. Well, village regrow blocker I guess? But that's fine to be a lookup based on an effect.

Regrow might have to be a type and an effect, where the effect points to the info about a lineage. Effect never clears, but if type is cleared, it no longer regrows. The effect is still required for correct money calculations after regrow stripping.