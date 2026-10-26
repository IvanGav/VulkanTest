#pragma once

#include "prelude.h"
#include "bloon.h"

namespace blist {

// https://www.youtube.com/watch?v=SHaAR7XPtNU
// Dense Slot Map datastructure
struct BList {
	Vec<nbloon::Bloon> list;
	Vec<BID> slots;
	u32 free_head; // U32_MAX means no free list

	static BList create() {
		return BList{ 
			.list = Vec<nbloon::Bloon>::with_capacity(&global_arena, 5000),
			.slots = Vec<BID>::with_capacity(&global_arena, 5000),
			.free_head = U32_MAX
		};
	}

	void clear() { list.clear(); slots.clear(); free_head = U32_MAX; }

	// Returned pointers are valid **only** before queued commands are applied (end of each frame)
    nbloon::Bloon* find(BID bid) {
		BID slot = slots[bid.i];
		if(slot.gen != bid.gen) return nullptr; // the requested bloon does not exist
		return &list[slot.i];
	}
	// Add a new bloon and return its assigned BID
    BID add(nbloon::Bloon b) {
		if(free_head == U32_MAX) {
			BID bid = BID{ .i = slots.size, .gen = 1 };
			b.id = bid;
			slots.push(bid);
			list.push(b);
			return bid;
		} else {
			u32 free_slot = free_head;
			free_head = slots[free_head].i; // advance free list head
			slots[free_slot].i = list.size; // we will push the new bloon to `bloons`
			BID bid = BID{.i = free_slot, .gen = slots[free_slot].gen };
			b.id = bid;
			list.push(b);
			return bid;
		}
	}
	// Delete the specified bloon and invalidate this BID. Return false if `bid` doesn't exist
	// Does not handle game logic
    bool del(BID bid) {
		BID slot = slots[bid.i];
		if(slot.gen != bid.gen) return false; // outdated generation
		if(slot.i == list.size-1) {
			list.pop();
		} else {
			list.remove_swap(slot.i);
			BID update_bid = list[slot.i].id;
			slots[update_bid.i].i = slot.i; // update the slot to point to the new location of this bloon
		}
		slots[bid.i].gen += 1; // invalidate old bid's to this bloon
		// add to the free list
		slots[bid.i].i = free_head;
		free_head = bid.i;
		return true;
	}

	/* STL Compatibility */
    nbloon::Bloon* begin() { return list.begin(); }
    nbloon::Bloon* end() { return list.end(); }
};

}