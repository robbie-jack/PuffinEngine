#pragma once

#include <cassert>
#include <vector>
#include <array>
#include <unordered_map>

namespace Puffin
{
	// Fixed and Dynamically Sized Arrays which ensure data is packed tightly to maximize cache utilization
	
	// Packed Fixed Array
	template<typename ValueT, size_t Size>
	class PackedArray
	{
	public:

		PackedArray()
		{
			m_arraySize = 0;
		}

		// Insert new Value into Array
		void Insert(size_t id, const ValueT& value)
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
		void Erase(size_t id)
		{
			assert(m_idToIndexMap.find(id) != m_idToIndexMap.end() && "Removing non-existent value");

			if (m_arraySize == 0 || m_idToIndexMap.count(id) == 0)
				return;

			// Copy value at end of array into deleted values space to maintain packed array
			size_t indexOfRemovedValue = m_idToIndexMap[id];
			size_t indexOfLastValue = m_arraySize - 1;
			m_array[indexOfRemovedValue] = m_array[indexOfLastValue];

			// Update map to point to values new location
			size_t idOfLastValue = m_indexToIDMap[indexOfLastValue];
			m_idToIndexMap[idOfLastValue] = indexOfRemovedValue;
			m_indexToIDMap[indexOfRemovedValue] = idOfLastValue;

			m_idToIndexMap.erase(id);
			m_idToIndexMap.erase(indexOfLastValue);

			m_arraySize--;
		}

		bool Contains(size_t id) const
		{
			return m_idToIndexMap.count(id) == 1;
		}

		void Clear()
		{
			m_idToIndexMap.clear();
			m_idToIndexMap.clear();
			m_arraySize = 0;
		}

		auto begin()
		{
			return m_array.begin();
		}

		auto end()
		{
			return m_array.end();
		}

		const ValueT& operator[](const size_t& id) const
		{
			assert(m_idToIndexMap.find(id) != m_idToIndexMap.end() && "No value with that id has been added to map");

			return m_array[m_idToIndexMap.at(id)];
		}

		ValueT& operator[](const size_t& id)
		{
			assert(m_idToIndexMap.find(id) != m_idToIndexMap.end() && "No value with that id has been added to map");

			return m_array[m_idToIndexMap[id]];
		}

	private:

		std::array<ValueT, Size> m_array; // Packed array of types
		size_t m_arraySize; // Number of valid entities in array

		std::unordered_map<size_t, size_t> m_idToIndexMap; // Map from id to index
		std::unordered_map<size_t, size_t> m_indexToIDMap; // Map from index to id
	};

	// Packed Dynamic Array
	template<typename ValueT>
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
		void Insert(size_t id, const ValueT& value)
		{
			assert(m_idToIndexMap.find(id) == m_idToIndexMap.end() && "Value with that ID already exists");

			// Insert Value at end of array
			size_t newIndex = m_vectorSize;
			m_idToIndexMap[id] = newIndex;
			m_indexToIDMap[newIndex] = id;
			m_vector.push_back(value);

			m_vectorSize++;
		}

		void Emplace(size_t id, const ValueT& value)
		{
			assert(m_idToIndexMap.find(id) == m_idToIndexMap.end() && "Value with that ID already exists");

			// Insert Value at end of array
			size_t newIndex = m_vectorSize;
			m_idToIndexMap[id] = newIndex;
			m_indexToIDMap[newIndex] = id;
			m_vector.emplace_back(value);

			m_vectorSize++;
		}

		// Remove value from Array
		void Erase(size_t id)
		{
			assert(m_idToIndexMap.find(id) != m_idToIndexMap.end() && "Removing non-existent value");

			if (m_vectorSize == 0 || m_idToIndexMap.count(id) == 0)
				return;

			// Copy value at end of array into deleted values space to maintain packed array
			size_t indexOfRemovedValue = m_idToIndexMap[id];
			size_t indexOfLastValue = m_vectorSize - 1;
			m_vector[indexOfRemovedValue] = m_vector[indexOfLastValue];

			// Delete value at end of vector
			m_vector.pop_back();

			// Update map to point to values new location
			size_t idOfLastValue = m_indexToIDMap[indexOfLastValue];
			m_idToIndexMap[idOfLastValue] = indexOfRemovedValue;
			m_indexToIDMap[indexOfRemovedValue] = idOfLastValue;

			m_idToIndexMap.erase(id);
			m_indexToIDMap.erase(indexOfLastValue);

			m_vectorSize--;
		}

		bool Contains(size_t id) const
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

		size_t Size() const
		{
			return m_vector.size();
		}

		auto begin()
		{
			return m_vector.begin();
		}

		auto end()
		{
			return m_vector.end();
		}

		const ValueT& operator[](const size_t& id) const
		{
			assert(m_idToIndexMap.find(id) != m_idToIndexMap.end() && "No value with that id has been added to map");

			return m_vector[m_idToIndexMap[id]];
		}

		ValueT& operator[](const size_t& id)
		{
			assert(m_idToIndexMap.find(id) != m_idToIndexMap.end() && "No value with that id has been added to map");

			return m_vector[m_idToIndexMap[id]];
		}

	private:
		
		std::vector<ValueT> m_vector; // Packed vector of types
		size_t m_vectorSize; // Number of valid entities in vector

		std::unordered_map<size_t, size_t> m_idToIndexMap; // Map from id to index
		std::unordered_map<size_t, size_t> m_indexToIDMap; // Map from index to id
	};

	// Custom bitset that ensures in use bits are packed together
	template<size_t Size>
	class PackedBitset
	{
	public:

		PackedBitset()
		{
			m_bitsetSize = 0;
		}

		void Insert(size_t id, const bool& value = false)
		{
			assert(m_idToIndexMap.find(id) == m_idToIndexMap.end() && "Value with that ID already exists");

			// Insert value at end of bitset
			size_t newIndex = m_bitsetSize;
			m_idToIndexMap[id] = newIndex;
			m_indexToIDMap[newIndex] = id;
			m_bitset[newIndex] = value;

			m_bitsetSize++;
		}

		// Remove value from bitset
		void Erase(size_t id)
		{
			assert(m_idToIndexMap.find(id) != m_idToIndexMap.end() && "Removing non-existent value");

			if (m_bitsetSize == 0 || m_idToIndexMap.count(id) == 0)
				return;

			// Copy value at end of array into deleted values space to maintain packed array
			size_t indexOfRemovedValue = m_idToIndexMap[id];
			size_t indexOfLastValue = m_bitsetSize - 1;
			m_bitset[indexOfRemovedValue] = m_bitset[indexOfLastValue];

			// Update map to point to values new location
			size_t idOfLastValue = m_indexToIDMap[indexOfLastValue];
			m_idToIndexMap[idOfLastValue] = indexOfRemovedValue;
			m_indexToIDMap[indexOfRemovedValue] = idOfLastValue;

			m_idToIndexMap.erase(id);
			m_indexToIDMap.erase(indexOfLastValue);

			m_bitsetSize--;
		}

		bool Contains(size_t id)
		{
			return m_idToIndexMap.count(id) == 1;
		}

		void Clear()
		{
			m_idToIndexMap.clear();
			m_idToIndexMap.clear();
			m_bitsetSize = 0;
		}

		typename std::bitset<Size>::reference operator[](size_t id)
		{
			return m_bitset[m_idToIndexMap[id]];
		}

	private:

		std::bitset<Size> m_bitset; // Internal Bitset
		size_t m_bitsetSize; // Number of bits currently occupied in bitset

		std::unordered_map<size_t, size_t> m_idToIndexMap; // Map from id to index
		std::unordered_map<size_t, size_t> m_indexToIDMap; // Map from index to id
	};
}