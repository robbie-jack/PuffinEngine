#pragma once

#include <map>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <entt/entity/registry.hpp>

// AngelScript Includes
#include "angelscript/angelscript.h"

#include "puffin/core/subsystem.h"
#include "puffin/core/engine.h"
#include "puffin/input/inputevent.h"
#include "puffin/types/ringbuffer.h"
#include "puffin/types/uuid.h"

#include "puffin/scripting/angelscript/angelscriptengineinterface.h"
#include "puffin/audio/audiosubsystem.h"
#include "puffin/physics/collisionevent.h"

namespace puffin::scripting
{
	class AngelScriptSubsystem : public core::Subsystem, public std::enable_shared_from_this<AngelScriptSubsystem>
	{
	public:

		AngelScriptSubsystem(const std::shared_ptr<core::Engine>& engine);
		~AngelScriptSubsystem() override;

		void Initialize(core::SubsystemManager* subsystem_manager) override;
		void Deinitialize() override;

		void EndPlay() override;

		void Update(double delta_time) override;
		bool ShouldUpdate() override;

		void on_construct_script(entt::registry& registry, entt::entity entity);
		void on_destroy_script(entt::registry& registry, entt::entity entity);

		bool prepare_and_execute_script_method(void* script_obj, asIScriptFunction* script_func);

		void set_current_entity_id(PuffinID id);

		// Hot-Reloads all scripts when called
		//void reload() {}

	private:

		asIScriptEngine* m_script_engine = nullptr;
		asIScriptContext* m_script_context = nullptr;

		std::unique_ptr<AngelScriptEngineInterface> m_engine_interface;

		PuffinID m_current_entity_id; // Entity ID for currently executing script

		// Event Buffers
		std::shared_ptr<RingBuffer<input::InputEvent>> m_input_events = nullptr;
		std::shared_ptr<RingBuffer<physics::CollisionBeginEvent>> m_collision_begin_events = nullptr;
		std::shared_ptr<RingBuffer<physics::CollisionEndEvent>> m_collision_end_events = nullptr;

		// Maps of Input Callbacks
		std::unordered_map<std::string, ScriptCallbackMap> m_on_input_pressed_callbacks;
		std::unordered_map<std::string, ScriptCallbackMap> m_on_input_released_callbacks;

		// Collision Callbacks
		ScriptCallbackMap m_on_collision_begin_callbacks;
		ScriptCallbackMap m_on_collision_end_callbacks;

		std::unordered_set<PuffinID> m_scripts_to_init;
		std::unordered_set<PuffinID> m_scripts_to_begin_play;
		std::unordered_set<PuffinID> m_scripts_to_end_play;

		void configure_engine();

		void init_context();
		void init_scripts();
		void start_scripts();
		void stop_scripts();

		void initialize_script(PuffinID entity, AngelScriptComponent& script);
		void compile_script(AngelScriptComponent& script) const;
		void update_script_methods(AngelScriptComponent& script);
		void instantiate_script_obj(PuffinID entity, AngelScriptComponent& script);

		void destroy_script(AngelScriptComponent& script);

		void process_events();

		asIScriptFunction* get_script_method(const AngelScriptComponent& script, const char* funcName);
		bool prepare_script_method(void* scriptObj, asIScriptFunction* scriptFunc);
		bool execute_script_method(void* scriptObj, asIScriptFunction* scriptFunc);

		// Global Script Functions
		[[nodiscard]] const double& get_delta_time() const;
		[[nodiscard]] const double& get_fixed_time() const;

		void play_sound_effect(uint64_t id, float volume = 1.0f, bool looping = false, bool restart = false);
		PuffinID play_sound_effect(const std::string& path, float volume = 1.0f, bool looping = false,
		                         bool restart = false);

		PuffinID get_entity_id(); // Return the Entity ID for the attached script

		// Script Callbacks
		ScriptCallback bind_callback(PuffinID entity, asIScriptFunction* cb) const;
		void release_callback(ScriptCallback& scriptCallback) const;

		// Collision Functions
		void bind_on_collision_begin(PuffinID entity, asIScriptFunction* cb);
		void bind_on_collision_end(PuffinID entity, asIScriptFunction* cb);

		void release_on_collision_begin(PuffinID entity);
		void release_on_collision_end(PuffinID entity);
	};

	class AngelScriptGameplaySubsystem : public core::Subsystem
	{
	public:

		AngelScriptGameplaySubsystem(std::shared_ptr<core::Engine> engine);
		~AngelScriptGameplaySubsystem() override = default;

		core::SubsystemType GetType() const override;

		void BeginPlay() override;
		void EndPlay() override;

		void Update(double delta_time) override;
		bool ShouldUpdate() override;

		void FixedUpdate(double fixed_time) override;
		bool ShouldFixedUpdate() override;

	private:



	};
}
