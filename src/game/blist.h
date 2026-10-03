#pragma once

#include "prelude.h"
#include "bloon.h"

namespace blist {

// https://www.youtube.com/watch?v=SHaAR7XPtNU
// Dense Slot Map datastructure
struct BList {
	Vec<nbloon::Bloon> bloons;
	Vec<nbloon::BID> slots;
	u32 free_head; // U32_MAX means no free list

	static BList create() {
		return BList{ 
			.bloons = Vec<nbloon::Bloon>::with_capacity(&global_arena, 5000),
			.slots = Vec<nbloon::BID>::with_capacity(&global_arena, 5000),
			.free_head = U32_MAX
		};
	}

	void clear() { bloons.clear(); slots.clear(); free_head = U32_MAX; }

	// Returned pointers are valid **only** before queued commands are applied (end of each frame)
    nbloon::Bloon* find(nbloon::BID bid) {
		nbloon::BID slot = slots[bid.i];
		if(slot.gen != bid.gen) return nullptr; // the requested bloon does not exist
		return &bloons[slot.i];
	}
	// Add a new bloon and return its assigned BID
    nbloon::BID add(nbloon::Bloon b) {
		if(free_head == U32_MAX) {
			nbloon::BID bid = nbloon::BID{ .i = slots.size, .gen = 1 };
			b.bid = bid;
			slots.push(bid);
			bloons.push(b);
			return bid;
		} else {
			u32 free_slot = free_head;
			free_head = slots[free_head].i; // advance free list head
			slots[free_slot].i = bloons.size; // we will push the new bloon to `bloons`
			nbloon::BID bid = nbloon::BID{.i = free_slot, .gen = slots[free_slot].gen };
			b.bid = bid;
			bloons.push(b);
			return bid;
		}
	}
	// Delete the specified bloon and invalidate this BID. Return false if `bid` doesn't exist
    bool del(nbloon::BID bid) {
		nbloon::BID slot = slots[bid.i];
		if(slot.gen != bid.gen) return false; // outdated generation
		if(slot.i == bloons.size-1) {
			bloons.pop();
		} else {
			bloons.remove_swap(slot.i);
			nbloon::BID update_bid = bloons[slot.i].bid;
			slots[update_bid.i].i = slot.i; // update the slot to point to the new location of this bloon
		}
		slots[bid.i].gen += 1; // invalidate old bid's to this bloon
		// add to the free list
		slots[bid.i].i = free_head;
		free_head = bid.i;
		return true;
	}
	// will delete bloon `bid`
	void pop(nbloon::BID bid) {
		nbloon::Bloon* bloon = this->find(bid);
		if (bloon == nullptr) { warn("tried popping a deleted bloon"); return; }
		for (u32 i = 0; i < bloon->num_children(); i++) {
			this->add(bloon->get_child(i));
		}
		this->del(bid);
	}

	/* STL Compatibility */
    nbloon::Bloon* begin() { return bloons.begin(); }
    nbloon::Bloon* end() { return bloons.end(); }
};

}