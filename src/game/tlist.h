#pragma once

#include "prelude.h"
#include "tower.h"

namespace tlist {

	struct TList {
		Vec<ntower::Tower> list;
		Vec<TID> slots;
		u32 free_head; // U32_MAX means no free list

		static TList create() {
			return TList{
				.list = Vec<ntower::Tower>::with_capacity(&global_arena, 300),
				.slots = Vec<TID>::with_capacity(&global_arena, 300),
				.free_head = U32_MAX
			};
		}

		void clear() { list.clear(); slots.clear(); free_head = U32_MAX; }

		// Returned pointers are valid **only** before queued commands are applied (end of each frame)
		ntower::Tower* find(TID id) {
			TID slot = slots[id.i];
			if (slot.gen != id.gen) return nullptr; // the requested item does not exist
			return &list[slot.i];
		}
		// Add a new item and return its assigned ID
		TID add(ntower::Tower t) {
			if (free_head == U32_MAX) {
				TID id = TID{ .i = slots.size, .gen = 1 };
				t.id = id;
				slots.push(id);
				list.push(t);
				return id;
			}
			else {
				u32 free_slot = free_head;
				free_head = slots[free_head].i; // advance free list head
				slots[free_slot].i = list.size; // we will push the new bloon to `bloons`
				TID id = TID{ .i = free_slot, .gen = slots[free_slot].gen };
				t.id = id;
				list.push(t);
				return id;
			}
		}
		// Delete the specified bloon and invalidate this BID. Return false if `bid` doesn't exist
		// Does not handle game logic
		bool del(TID id) {
			TID slot = slots[id.i];
			if (slot.gen != id.gen) return false; // outdated generation
			if (slot.i == list.size - 1) {
				list.pop();
			}
			else {
				list.remove_swap(slot.i);
				TID update_id = list[slot.i].id;
				slots[update_id.i].i = slot.i; // update the slot to point to the new location of this bloon
			}
			slots[id.i].gen += 1; // invalidate old bid's to this bloon
			// add to the free list
			slots[id.i].i = free_head;
			free_head = id.i;
			return true;
		}

		/* STL Compatibility */
		ntower::Tower* begin() { return list.begin(); }
		ntower::Tower* end() { return list.end(); }
	};

}