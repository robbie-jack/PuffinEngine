//#pragma once
//
//#include <ECS/ECS.h>
//#include <SFML/Audio.hpp>
//
//#include <Types/RingBuffer.h>
//
//#include <string>
//#include <string_view>
//#include <vector>
//#include <unordered_map>
//#include <memory>
//
//namespace Puffin
//{
//	namespace Audio
//	{
//		// Event to load audio from file
//		struct LoadSoundEvent
//		{
//			LoadSoundEvent(std::string InSoundName = "", std::string InSoundFilePath = "") 
//				: soundName{ InSoundName }, soundFilePath{ InSoundFilePath } {};
//
//			std::string soundName;
//			std::string soundFilePath;
//		};
//
//		// Enum of what action a sound should perform
//		enum class SoundAction
//		{
//			Stop = 0,
//			Play = 1,
//			Pause = 2
//		};
//
//		// Event for Playing/Pausing/Stopping Sounds
//		// Can also pass variables to alter sound playback, like volume, pitch, etc...
//		struct SoundEvent
//		{
//			SoundEvent(std::string InSoundName = "", SoundAction InSoundAction = SoundAction::Stop)
//				: soundName{ InSoundName }, soundAction{ InSoundAction } {};
//
//			std::string soundName;
//			SoundAction soundAction;
//		};
//
//		class AudioSystem : ECS::System
//		{
//		public:
//
//			void Init();
//
//			void Update(float dt);
//
//			void Cleanup();
//
//		private:
//
//			void ProcessEvents();
//
//			// Map from the sound's name to its index
//			std::unordered_map<std::string_view, int> soundNameToIndexMap;
//
//			// Vector of loaded sound buffers
//			std::vector<sf::SoundBuffer> soundBuffers;
//
//			std::shared_ptr<RingBuffer<LoadSoundEvent>> loadSoundEvents;
//		};
//	}
//}