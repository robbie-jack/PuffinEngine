#include "AudioSystem.h"

#include <iostream>

namespace Puffin
{
	namespace Audio
	{
		void AudioSystem::Init()
		{
			loadSoundEvents = std::make_shared<RingBuffer<LoadSoundEvent>>();

			world->RegisterEvent<LoadSoundEvent>();
			world->SubscribeToEvent<LoadSoundEvent>(loadSoundEvents);
		}

		void AudioSystem::Update(float dt)
		{
			ProcessEvents();
		}

		void AudioSystem::Cleanup()
		{

		}

		void AudioSystem::ProcessEvents()
		{
			// Process Load Audio Events
			LoadSoundEvent loadSoundEvent;
			while (!loadSoundEvents->IsEmpty())
			{
				if (loadSoundEvents->Pop(loadSoundEvent))
				{
					
					if (soundNameToIndexMap.find(loadSoundEvent.soundName) == soundNameToIndexMap.end())
					{
						// There is already a sound effect with that name
						std::cout << "Sound with that name already exists" << std::endl;
					}
					else
					{
						// Create new sound buffer
						sf::SoundBuffer buffer;

						// Load sound from file
						if (!buffer.loadFromFile(loadSoundEvent.soundFilePath))
						{
							// Sound file failed to load, print error message
							std::cout << "Sound file failed to load" << std::endl;
						}
						else
						{
							// Sound succeeded in loading, add it to vector
							soundBuffers.push_back(buffer);

							// Add sound name and index into buffer vector to map
							soundNameToIndexMap.insert({ loadSoundEvent.soundName, soundBuffers.size() - 1});
						}
					}
				}
			}

			// Process Playing/Pause events
		}
	}
}