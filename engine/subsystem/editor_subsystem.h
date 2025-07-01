#pragma once

#include "subsystem/engine_subsystem.h"

namespace puffin
{
	namespace core
	{
		class EditorSubsystem : public EngineSubsystem
		{
		public:

			EditorSubsystem(std::shared_ptr<Engine> engine);
			~EditorSubsystem() override;

		private:



		};
	}

	namespace reflection
	{
		template<>
		inline std::string_view GetTypeString<core::EditorSubsystem>()
		{
			return "EditorSubsystem";
		}

		template<>
		inline entt::hs GetTypeHashedString<core::EditorSubsystem>()
		{
			return entt::hs(GetTypeString<core::EditorSubsystem>().data());
		}

		template<>
		inline void RegisterType<core::EditorSubsystem>()
		{
			auto meta = entt::meta<core::EditorSubsystem>()
				.base<core::EngineSubsystem>()
				.base<core::Subsystem>();

			RegisterTypeDefaults(meta);
		}
	}
}