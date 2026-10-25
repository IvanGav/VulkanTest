dart monkey
- catapult
- super monkey fan club
- crossbow master

boomerang monkey
- bouncing boomerangs (change target on hit)
- robo boomer
- kyle (weird movement)

bomb shooter
- stun bombs
- moab mauler
- bomb blitz

tack shooter
- tack zone
- maelstorm (on ability, suspend regular attack)
- ring of fire

ice monkey (water/land)
- +damage debuff
- snowstorm (provide standing spots on nearby water)
- icicle cannon

glue gunner
- bloon solver
- glue storm
- sticky glue

___

sniper monkey
- cripple moab (insta hit attack)
- delivery + ricochet (redirect on bloon hit + shapnel on projectile death; additional target modes based on monkey present)
- full auto (ability trigger on life lost; faster speed on bloon closest to front)

monkey sub (water)
- reactor/radar (target mode entirely changes animation and attack; can target bloon in any monkey's range)
- first strike (blimp overkill)
- armor piercing rounds

monkey buccaneer (water)
- destroyer/fleet (provide standing spots for new monkeys)
- pirate lord
- trade empire (on round end give money/generic behavior)

monkey ace (subtower that moves only during rounds)
- lots of darts
- tzar bomba (detect when subtower is over track)
- flying fortress

heli pilot
- attack heli (slowly move to mouse)
- support chinock (ability is not instant, but instead orders a subtower to move and then execute place/pickup when reached the spot)
- apachi (track progress % of farthest bloon and keep internal supporter heli counter per round)

mortar monkey (target mode of target mouse, set target)
- nuke
- fast mortar
- blooncinderation (strip camo/fortified)

dartling gunner (turn to target (aka mouse or bloon) during rounds instead of insta turn)
- ROD (ray of doom) (ray projectile)
- MAD
- BEZ (Bloon Exclusion Zone) (several independent non-insta-rotating platforms aka multiple rotations aka probably subtowers?)

___

wizard monkey
- archmage
- phoenix
- necromancer (spawn attacks that have "along path" movement)

ninja monkey
- grand master
- grand sabo
- sticky bomb

alchemist monkey
- alch buff (attack targets monkeys in range to give buff)
- transformation
- rubber to gold (debuff that modified bloon value + projectile/debuff/system that replaces bloon with different bloon)

druid monkey
- avatar of wrath
- a
- jungle of the druid

mermonkey (water/land)
- tentacle lord (8 directional attackers with unique animations)
- freeze lord (projectile hitbox changes based on time alive) // aka really bad news
- final harmonic (subtower that modifies (and remembers) paths of bloons)

super monkey
- sun god
- robo
- legend of the night

___

banana farm ("projectiles" that respond to mouse instead of bloons)
- banana central
- bank
- central market

spike factory
- super mines
- carpet of spikes
- perma spike (projectiles live between rounds)

monkey village
- primary expertise
- homeland defence
- monkeyopolis

water shooter (water)
- slowdown master
- knockback master (ability knocks everything back massively)
- damage increase salty water

engineer monkey
- trap -> nerfed bloon chipper
- overclock
- sentry champion -> debuff focused

___

Common spcial needs:
- Count number of (specific) towers in a range
- Do something to towers in range
- Projectiles that can hit only targeted bloon
- Projectiles reset their hit list every game tick (aka don't record a hit list and hit everything they intersect without previous hit check)
- On bloon about to exit, trigger an ability/attack
- Ability doesn't reset cooldown until a trigger has passed
- Replace monkey with a different one for limited time (different visuals, attacks)
- On bloon pop, spawn projectile
- Apply bloon debuff only if same type of debuff doesn't exist
- Bloon buff that clamps speed (min,max) - may have stun as clamp(0,0)
- Visual effects on bloon buffs
- Bloon buffs should define how they behave (stay, removed) on bloon pop. Behavior may be different between bloons and blimps
- Subtowers may look up and follow the main tower's target mode

___

Normal monkey counters:
- Pop count - statistic

Special monkey counters:
- Additional money made (on money making monkeys) - statistic
- Sun god sacrifices (manually set by external function calls and does pretty much nothing otherwise) - misc, one off
- Undead power + max undead power (aka number of bloons who died in range; only count for necromancers) - for attacks
- Shots fired (crit) counter - for attacks
- Target position (for certain primary or secondary attacks) - for attacks

Very special monkey state:
- Entire shop state (corvus, geraldo) - may have a nullable pointer to this state
- Mana (corvus)

___

More notes:
- I don't actually like regrow bloons all that much. They are usually not in the slightest an issue. And when they are, they're absolutely unbeatable. Other times they're abused to make regrow farms. So what if I made them regrow up to, say, 10 times. That would solve so many things. Nerf them. Eliminate boring and tedious regrow farms. Eliminate my issue with BFT limit of 32/64 bits. Genious.
- Subtowers are a bit strange. I think every time without fail, they give own pops to the main tower. They can often move (between rounds in base game), so make sure to freeze between roudns. The logic of how to distribute buffs between main/subtower is a bit complicated. Can probably be defined by each subtower, instad of each main tower mhm.
- Bloon (de)buffs should be able to affect the amount of damage a bloon takes, so a bloon's own buff context should be calculated and used during hit detection and hit application. During hit detection, because the bloon *may* gain temporary camo status through a buff or something.
- Glue is strange. It's an antisynnergy with itself. Only one glue type can be active at once. So they can overwrite each other in a horrifically complex priority system. Couple of solutions. Can make glue from each path apply independently (and strictly weaker glue gets overwritten by strictly stronger one in the same path). That makes the glue quite damn op. Alternatively, can make my own uncomplicated glue priority system. I like glue. I'm biased to just make it op.
- To count pops, all projectiles will need a back reference to the tower that shot them. That will, in turn, require monkeys to have BID of their own (aka TID or MID; more generally GID - generational id). Which is needed anyway for subtowers and such.
- When rubber to gold'd bloon gets popped, the monkey that popped it doesn't "generate additional cash". The alchemist monkey does.
- How do shooty turrets and engi turrets and such separately selectable "subtowers" interact with main tower's pops? Do they dupe the pops or keep to themselves or siphon to main tower?
 - Depending on how I decide this should work, there may be several types of subtowers: true subtowers (cannot be selected separately, despawn when main tower depanws), child towers (respect target mode and buffs of main tower, but separate otherwise), spawned towers (independent towers). Or something along those lines, based on specific specification I go with.
- If/When I decide to implement heroes and paragons, they will all require a *very* special handling, state and everything. In fact so special sometimes (think geraldo, corvus), that I might want to have a polymorphic `Tower` class with base class being responsible for 98% of all towers. That will get inconvenient though, as I don't want to have a tripple inderection tower lookup (TID->index table, index table->pointer table, pointer->object). It won't be a huge issue, but I also don't want to have the darn arena eating towers. Having a "special_info*" that's usually `nullptr` is probably better, for those *very* special cases.
- Depending on how I decide to make zombie bloons work, I may need to introduce a new way of damaging bloons - 1 pierce = 1 damage, spend as much pierce as needed to bring down to 0 hp. Although that sounds too strong, so maybe just make it work like the spikes.
- I will sometimes (necromancer, wall of fire, etc) need a "special target" to target a secondary attack. Sometimes it'll need to be targetable anywhere, sometimes only path. Or just take closest path, but either way I'll need to *store* the path sometimes for optimization. It can also (sometimes?) be "unset" to use default behavior.
- I don't know if I should hardcode the 5-2-0 of 5-5-5 upgrade system. Because it *could* be cool to have it non-fully-hardcoded. But at the same time it would be much simpler if it *was* hardcoded. The easiest solution that gets best of both worlds is just having a handler function for 5-2-0 of 5-5-5 and having an if statement checking what upgrade schema the given tower has.

___

Additional features:
- As much as I hate Battle Cats' metal enemies, it's kinda neat. And, BTD6 eventually introduced them. But I'd like to have a custom type/game mode with "cat metal" bloons that take 1 damage a hit.
- I want to have a PVZ Fusion inspired "monkey merge" mechanic as a secret (sauda + plane = ISAB plane) or additional game mode feature. When monkey is dropped over another one, a new monkey spawns instead. Maybe let merge monkeys that are already on the field, kind of like beastiality handler.
- Probably do want paragons, but different from BTD6 paragons because I don't love how those are implemented. Would be kind of cool to get a flavor of your paragon based on which of 3 paths you do upgrade.
- Probably also want heroes, but not all same as BTD6 because some of them are lame (corvus) and some are OP (geraldo).
- Probably want some special game modes. Like Rogue Legends but actually fun. What's gonna make it fun is a very game dev question that I don't know the answer to, but definitely more optimized and not paywall'd.

___

Special events (when you can easily run arbitrary code with exclusive world access):
- Buy monkey
- Upgrade monkey
- Sell monkey
- Use ability (some of them are kind of crazy)
- Bloon pop? (necromancer)
