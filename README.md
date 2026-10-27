dart monkey
- catapult
- super monkey fan club
- crossbow master

boomerang monkey
- bouncing boomerangs
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

sniper monkey (insta hit attack)
- cripple moab
- delivery + ricochet (additional target modes based on monkey present)
- full auto (faster speed on bloon closest to front)

monkey sub (water)
- reactor/radar (target mode entirely changes animation and attack; can target bloon in any monkey's range)
- first strike (blimp overkill)
- armor piercing rounds

monkey buccaneer (water)
- destroyer/fleet (provide standing spots for new monkeys)
- pirate lord
- trade empire

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
- thunder druid (projectile that bounces from screen borders)
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
- Projectile changes target on hit
- Check position along track of first/farthest bloon

___

Normal monkey counters:
- Pop count - statistic
- Additional money made (on money making monkeys) - statistic, fairly common

Special monkey counters:
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
- As much as I'd like to, I should never rely on raw pointer comparison for equality. When saving/loading state, it'd be *easiest* if I didn't have to restore the exact memory state up to raw pointer equivalence. So the only case where it may be ok is comparing `Proto`s, since those may be looked up and reverse looked up by their key. And that's not even that useful anyway. After that, I need a function to load every kind of runtime object (non-`Proto`s, of course). And then a loop over all game structures to serialize in order, and then deserialize in order.

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
- Round start
- Round end

___

Yapping:
- Interesting problem when I need to call `before_bloon_exited` and `after_bloon_exited` event handlers. Some of them may (and usually will) damage literally all bloons on the screen. So I'll need to continue handling all non-exited bloons to apply overkill and stuff from this tick. And then, I'll need to apply the events and THEN I'll need to go over all bloons again, since any amount of bloons may have been damaged/popped. I specifically do *not* need to apply any other `updateTick` stages, only the "pop bloons" stage, iteratively, until no event handlers have been called.
- There's also a chance that having the Necromancer undead power and shots fired for Crits is better as part of an attack. Since it's inhenently the data that's used for an *attack*. Which turns my attacks from a lightweight `usize+u32` into either a polymorphic type (not happening) or a megastruct. Which is not horrible, but a bit annoying..
- Overkilling bloons is actually deceivingly hard. You have to calculate a list of all children in one go, spawn them in a single line (aka iteratively popping and pushing on track is insufficient). But you also should apply the effects and calculate the true health instead of using the base health (unless that's what I want to have?).
- Defortifying must also correct attribute all *child* fortified pops to the defortifying tower...

Ok actually though. How do I handle bloon overkill? The problem is this:
There may be (and will be - spike factory) several projectiles in the same tick that damage the same bloon.
I need to handle overkill on the parent bloon, including the fortified property of child bloons, and calculate the consistent final damage.
I may want to calculate the "optimal" damage - which means counting the non-overkill-able damage and overkill-able damage separately and apply them in order.
Regardless, it needs to be deterministic even if you reload the save... Yeah, I probably want to calculate damage with and without overkill separately.
After that I need to make sure to not overkill past their last child bloon. Or I mean, I can do it I guess, numerically.
But later when applying the popping with overkill, I need to *attribute* the damage correctly to each projectile that hit that bloon.
Again, it needs to be deterministic. When spawning the child bloons and applying all of the overkills and stuff.
Wait, the non-overkill damage can be attributed straight at the point before popping the bloon.
Ok, what are the most common cases? Blimp and non-overkill damage. Bloon and overkill damage (duh).
So. Non-overkill damage gets applied immidiately.
Overkill damage gets applied separately during the pop-overkill phase. Because, again, I need to attribute the damage values to the right tower and make sure 
I don't over-overkill a bloon.

Ok also. What are the tiebreakers on what projectile gets to damage the bloon first (hp/pop issue) *and* what bloons the projectile gets to damage first (pierce issue).
The obvious answer is "who was created/spawned first". Another answer is "who has more damage/hp" - but still requires tiebreaking ugh.

Damn. This is actually quite hard. Simply the fact that I need to *attribute* pops to the right towers makes it so *so* much harder.