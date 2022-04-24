#pragma once

#include <cassert>
#include <vector>
#include <array>
#include <unordered_map>

namespace Puffin
{
	// Fixed and Dynamically Sized Arrays which ensure data is packed tightly to maximize cache utilization
	
	// Packed Fixed Array
	template<typename IdT, typename ValueT, size_t Size>
	class PackedArray
	{
	public:

		PackedArray()
		{
			m_arraySize = 0;
		}

		// Insert new Value into Array
		void Insert(IdT id, ValueT value)
		{
			assert(m_idToIndexMap.find(id) == m_idToIndexMap.end() && "Value with that ID already exists");

			// Insert Value at end of array
			size_t newIndex = m_arraySize;
			m_idToIndexMap[id] = newIndex;
			m_indexToIDMap[newIndex] = id;
			m_array[newIndex] = value;

			m_arraySize++;
		}

		// Remove value from Array
		void Erase(IdT id)
		{
			assert(m_idToIndexMap.find(id) == m_idToIndexMap.end() && "Removing non-existent value");

			// Copy value at end of array into deleted values space to maintain packed array
			size_t indexOfRemovedValue = m_idToIndexMap[id];
			size_t indexOfLastValue = m_arraySize - 1;
			m_array[indexOfRemovedValue] = m_array[indexOfLastValue];

			// Update map to point to values new location
			IdT idOfLastValue = m_indexToIDMap[indexOfLastValue];
			m_idToIndexMap[idOfLastValue] = indexOfRemovedValue;
			m_indexToIDMap[indexOfRemovedValue] = idOfLastValue;

			m_idToIndexMap.erase(id);
			m_idToIndexMap.erase(indexOfLastValue);

			m_arraySize--;
		}

		bool Contains(IdT id)
		{
			return m_idToIndexMap.count(id) == 1;
		}

		void Clear()
		{
			m_idToIndexMap.clear();
			m_idToIndexMap.clear();
			m_arraySize = 0;
		}

		const ValueT& operator[](IdT id) const
		{
			assert(m_idToIndexMap.find(id) != m_idToIndexMap.end() && "No value with that id has been added to map");

			return m_array[m_idToIndexMap[id]];
		}

		ValueT& operator[](IdT id)
		{
			assert(m_idToIndexMap.find(id) != m_idToIndexMap.end() && "No value with that id has been added to map");

			return m_array[m_idToIndexMap[id]];
		}

	private:

		std::array<ValueT, Size> m_array; // Packed array of types
		size_t m_arraySize; // Number of valid entities in array

		std::unordered_map<IdT, size_t> m_idToIndexMap; // Map from id to index
		std::unordered_map<size_t, IdT> m_indexToIDMap; // Map from index to id
	};

	// Packed Dynamic Array
	template<typename IdT, typename ValueT>
	class PackedVector
	{
	public:

		PackedVector()
		{
			m_vectorSize = 0;
		}

		// Initialize Packed Vector with number of items to reserve space for
		PackedVector(size_t initialSize) : PackedVector()
		{
			m_vector.reserve(initialSize);
		}

		// Insert new Value into Array
		void Insert(IdT id, ValueT value)
		{
			assert(m_idToIndexMap.find(id) == m_idToIndexMap.end() && "Value with that ID already exists");

			// Insert Value at end of array
			size_t newIndex = m_vectorSize;
			m_idToIndexMap[id] = newIndex;
			m_indexToIDMap[newIndex] = id;
			m_vector.push_back(value);

			m_vectorSize++;
		}

		ValueT& Insert(IdT id, ValueT value)
		{
			Insert(id, value);
			return m_vector[id];
		}

		// Remove value from Array
		void Erase(IdT id)
		{
			assert(m_idToIndexMap.find(id) == m_idToIndexMap.end() && "Removing non-existent value");

			// Copy value at end of array into deleted values space to maintain packed array
			size_t indexOfRemovedValue = m_idToIndexMap[id];
			size_t indexOfLastValue = m_vectorSize - 1;
			m_vector[indexOfRemovedValue] = m_vector[indexOfLastValue];

			// Delete value at end of vector
			m_vector.pop_back();

			// Update map to point to values new location
			IdT idOfLastValue = m_indexToIDMap[indexOfLastValue];
			m_idToIndexMap[idOfLastValue] = indexOfRemovedValue;
			m_indexToIDMap[indexOfRemovedValue] = idOfLastValue;

			m_idToIndexMap.erase(id);
			m_idToIndexMap.erase(indexOfLastValue);

			m_vectorSize--;
		}

		bool Contains(IdT id)
		{
			return m_idToIndexMap.count(id) == 1;
		}

		void Clear()
		{
			m_idToIndexMap.clear();
			m_idToIndexMap.clear();
			m_vector.clear();
			m_vectorSize = 0;
		}

		void Reserve(size_t size)
		{
			m_vector.reserve(size);
		}

		const ValueT& operator[](IdT id) const
		{
			assert(m_idToIndexMap.find(id) != m_idToIndexMap.end() && "No value with that id has been added to map");

			return m_vector[m_idToIndexMap[id]];
		}

		ValueT& operator[](IdT id)
		{
			assert(m_idToIndexMap.find(id) != m_idToIndexMap.end() && "No value with that id has been added to map");

			return m_vector[m_idToIndexMap[id]];
		}

	private:
		
		std::vector<ValueT> m_vector; // Packed vector of types
		size_t m_vectorSize; // Number of valid entities in vector

		std::unordered_map<IdT, size_t> m_idToIndexMap; // Map from id to index
		std::unordered_map<size_t, IdT> m_indexToIDMap; // Map from index to id
	};
}