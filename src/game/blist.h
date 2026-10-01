#pragma once

#include "prelude.h"
#include "bloon.h"

namespace blist {

// https://www.youtube.com/watch?v=SHaAR7XPtNU
// Dense Slot Map datastructure
struct BList {
	Vec<bloon::Bloon> bloons;
	Vec<bloon::BID> slots;
	u32 free_head; // U32_MAX means no free list

	static BList create() { return BList{.bloons = Vec<bloon::Bloon>::with_capacity(5000), .slots = Vec<bloon::BID>::with_capacity(5000), .free_head = U32_MAX}; }

	void clear() { bloons.clear(); slots.clear(); free_head = U32_MAX; }

	// Returned pointers are valid **only** before queued commands are applied (end of each frame)
    bloon::Bloon* find(bloon::BID bid) {
		bloon::BID slot = slots[bid.i];
		if(slot.gen != bid.gen) return nullptr; // the requested bloon does not exist
		return &bloons[slot.i];
	}
	// Add a new bloon and return its assigned BID
    bloon::BID add(bloon::Bloon b) {
		if(free_head == U32_MAX) {
			bloon::BID bid = bloon::BID{ .i = slots.size, .gen = 1 };
			b.bid = bid;
			slots.push(bid);
			bloons.push(b);
			return bid;
		} else {
			u32 free_slot = free_head;
			free_head = slots[free_head].i; // advance free list head
			slots[free_slot].i = bloons.size; // we will push the new bloon to `bloons`
			bloon::BID bid = bloon::BID{.i = free_slot, .gen = slots[free_slot].gen };
			b.bid = bid;
			bloons.push(b);
			return bid;
		}
	}
	// Delete the specified bloon and invalidate this BID. Return false if `bid` doesn't exist
    bool del(bloon::BID bid) {
		bloon::BID slot = slots[bid.i];
		if(slot.gen != bid.gen) return false; // outdated generation
		if(slot.i == bloons.size-1) {
			bloons.pop();
		} else {
			bloons.remove_swap(slot.i);
			bloon::BID update_bid = bloons[slot.i].bid;
			slots[update_bid.i].i = slot.i; // update the slot to point to the new location of this bloon
		}
		slots[bid.i].gen += 1; // invalidate old bid's to this bloon
		// add to the free list
		slots[bid.i].i = free_head;
		free_head = bid.i;
		return true;
	}

	/* STL Compatibility */
    bloon::Bloon* begin() { return bloons.begin(); }
    bloon::Bloon* end() { return bloons.end(); }
};

}