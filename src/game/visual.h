#pragma once

#include "global.h"
#include "../graphics/engine.h"

namespace visual {

	enum class AnimationState {
		Spawn, Idle, Attack, ENUM_SIZE
	};

	enum class AnimationTransitionType {
		Allow, AllowWhenDone, Disallow
	};

	typedef u32 AnimationTableRef;

	Vec<std::array<u32, u32(AnimationState::ENUM_SIZE)>> animation_duration_table;
	Vec<std::array<std::array<AnimationTransitionType, u32(AnimationState::ENUM_SIZE)>, u32(AnimationState::ENUM_SIZE)>> animation_transition_table;
	Vec<std::array<AnimationState, u32(AnimationState::ENUM_SIZE)>> animation_auto_transition_table;

	struct Static {
		engine::MeshRef mesh;
		engine::TextureRef texture;

		bool is_billboard() const { return mesh.size < 7; } // TODO this is a stupid implementation, fix later when I actually have stuff
	};
	struct Animated {
		engine::MeshRef mesh;
		engine::TextureRef texture;

		AnimationState state;
		AnimationTableRef animation_duration_ref;
		AnimationTableRef animation_transition_ref;

		u32 animation_transition_at;

		Animated instanciate() {
			Animated a = *this;
			a.animation_transition_at = data::update;
			return a;
		}

		// [0,1]
		f32 animation_progress() const {
			u32 animation_duration_ticks = data::update - animation_transition_at;
			u32 animation_duration = animation_duration_table[animation_duration_ref][u32(state)];
			return glm::clamp(f32(animation_duration_ticks) / animation_duration, 0.0f, 1.0f);
		}

		bool transition(AnimationState new_state) {
			AnimationTransitionType type = animation_transition_table[animation_transition_ref][u32(state)][u32(new_state)];
			if (type == AnimationTransitionType::Disallow || (type == AnimationTransitionType::AllowWhenDone && animation_progress() != 1.0f)) {
				return false;
			}
			animation_transition_at = data::update;
			state = new_state;
			return true;
		}

		// transition table from old to new animation must not be Disallow
		void do_auto_transition() {
			if (animation_progress() == 1.0f) {
				bool success = transition(animation_auto_transition_table[animation_transition_ref][u32(state)]);
				assert(success);
			}
		}
	};
}