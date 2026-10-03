#pragma once

#include "proj.h"

namespace plist {

constexpr u32 WINDOW_SIZE = 16;

struct HitChunk {
    nbloon::BFT hit_bloons[WINDOW_SIZE];
    u32 next_chunk_i = U32_MAX; // Linked list to previous 16 hits
    u8 count = 0;
};

struct HitLedger {
    Vec<HitChunk> pool;
    u32 free_list;

    u32 allocate_chunk() {
        if(free_list == U32_MAX) {
            pool.push(HitChunk{});
            return pool.size - 1;
        }
        u32 free_chunk_i = free_list;
        free_list = pool[free_list].next_chunk_i;
        pool[free_chunk_i] = HitChunk{ .next_chunk_i = U32_MAX };
        return free_chunk_i;
    }

    // Record the hit and return the new chunk index to store in the projectile
    u32 record_hit(u32 i, nbloon::BFT bft) {
        if(i == U32_MAX) {
            i = this->allocate_chunk();
        } else if(pool[i].count == WINDOW_SIZE) {
            u32 prev_i = i;
            i = this->allocate_chunk();
            pool[i].next_chunk_i = prev_i;
        }
        pool[i].hit_bloons[pool[i].count] = bft;
        pool[i].count += 1;
        return i;
    }

    void free_chain(u32 head_i) {
        u32 cur = head_i;
        while(cur != U32_MAX) {
            u32 next = pool[cur].next_chunk_i;
            pool[cur].next_chunk_i = free_list;
            free_list = cur;
            cur = next;
        }
    }

    // return true if can hit `bft` (it/its parent hasn't been hit before)
    bool can_hit(u32 i, nbloon::BFT bft) {
        while (i != U32_MAX) {
            HitChunk& chunk = pool[i];
            for (u8 iter = 0; iter < chunk.count; iter++) {
                if (bft.same_subtree_as(chunk.hit_bloons[iter])) { return false; }
            }
            i = chunk.next_chunk_i;
        }
        return true;
    }
};

//struct PList {
//    HitLedger hits;
//    Vec<nproj::Projectile> projs;
//    Vec<nproj::PID> slots;
//    u32 free_head; // U32_MAX means no free list
//
//    static PList create() {
//        return PList{
//            .projs = Vec<nproj::Projectile>::with_capacity(&global_arena, 10000),
//            .slots = Vec<nproj::PID>::with_capacity(&global_arena, 10000),
//            .free_head = U32_MAX
//        };
//    }
//
//    void clear() { projs.clear(); slots.clear(); free_head = U32_MAX; }
//
//    // Returned pointers are valid **only** before queued commands are applied (end of each frame)
//    nproj::Projectile* find(nproj::PID pid) {
//        nproj::PID slot = slots[pid.i];
//        if (slot.gen != pid.gen) return nullptr; // the requested bloon does not exist
//        return &projs[slot.i];
//    }
//    // Add a new bloon and return its assigned BID
//    nproj::PID add(nproj::Projectile p) {
//        if (free_head == U32_MAX) {
//            nproj::PID pid = nproj::PID{ .i = slots.size, .gen = 1 };
//            p.pid = pid;
//            slots.push(pid);
//            projs.push(p);
//            return pid;
//        }
//        else {
//            u32 free_slot = free_head;
//            free_head = slots[free_head].i; // advance free list head
//            slots[free_slot].i = projs.size; // we will push the new bloon to `bloons`
//            nproj::PID pid = nproj::PID{ .i = free_slot, .gen = slots[free_slot].gen };
//            p.pid = pid;
//            projs.push(p);
//            return pid;
//        }
//    }
//    // Delete the specified proj and invalidate this PID. Return false if `pid` doesn't exist
//    bool del(nproj::PID pid) {
//        nproj::PID slot = slots[pid.i];
//        if (slot.gen != pid.gen) return false; // outdated generation
//        if (slot.i == projs.size - 1) {
//            projs.pop();
//        }
//        else {
//            projs.remove_swap(slot.i);
//            nproj::PID update_pid = projs[slot.i].pid;
//            slots[update_pid.i].i = slot.i; // update the slot to point to the new location of this proj
//        }
//        slots[pid.i].gen += 1; // invalidate old pid's to this proj
//        // add to the free list
//        slots[pid.i].i = free_head;
//        free_head = pid.i;
//        return true;
//    }
//
//    /* STL Compatibility */
//    nproj::Projectile* begin() { return projs.begin(); }
//    nproj::Projectile* end() { return projs.end(); }
//};

struct PList {
    HitLedger hits;
    Vec<nproj::Projectile> list;

    static PList create() { return PList{ .hits = HitLedger{.pool = {}, .free_list = U32_MAX }, .list = Vec<nproj::Projectile>::with_capacity(&global_arena, 10000) }; }

    void add(nproj::Projectile p) {
        p.hit_bloons_i = U32_MAX; // hits.allocate_chunk();
        list.push(p);
	}
    void del(u32 i) {
        hits.free_chain(list[i].hit_bloons_i);
        list.remove_swap(i);
	}
    void record_hit(u32 i, nbloon::BFT bft) {
        list[i].hit_bloons_i = hits.record_hit(list[i].hit_bloons_i, bft);
    }
    bool can_hit(u32 i, nbloon::BFT bft) {
        return hits.can_hit(list[i].hit_bloons_i, bft);
    }
    
	/* STL Compatibility */
    nproj::Projectile* begin() { return list.begin(); }
    nproj::Projectile* end() { return list.end(); }
};

}