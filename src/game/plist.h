#pragma once

#include "proj.h"

namespace plist {

constexpr u32 WINDOW_SIZE = 16;

struct HitChunk {
    bloon::BFT hit_bloons[WINDOW_SIZE];
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
        pool[free_chunk_i] = HitChunk{};
        return free_chunk_i;
    }

    // Record the hit and return the new chunk index to store in the projectile
    u32 record_hit(u32 i, bloon::BFT bft) {
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
    bool can_hit(u32 i, bloon::BFT bft) {
        // TODO maybe make non-recursive
        HitChunk& chunk = pool[i];
        for(u8 iter = 0; iter < chunk.count; iter++) {
            if(bft.same_subtree_as(chunk.hit_bloons[iter])) { return false; }
        }
        if(chunk.next_chunk_i == U32_MAX) return true;
        return this->can_hit(chunk.next_chunk_i, bft);
    }
};

struct PList {
    HitLedger hits;
    Vec<proj::Projectile> list;

    static PList create() { return PList{.list = Vec<proj::Projectile>::with_capacity(10000)}; }

    void add(proj::Projectile p) {
        p.hit_bloons_i = U32_MAX; // hits.allocate_chunk();
        list.push(p);
	}
    bool del(u32 i) {
        hits.free_chain(list[i].hit_bloons_i);
        list.remove_swap(i);
	}
    
	/* STL Compatibility */
    proj::Projectile* begin() { return list.begin(); }
    proj::Projectile* end() { return list.end(); }
};

}