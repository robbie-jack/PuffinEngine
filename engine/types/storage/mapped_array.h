#pragma once

#include <cassert>
#include <vector>
#include <array>
#include <unordered_map>
#include <bitset>

namespace puffin
{
	/*
	 * Array where items are packed consecutively for optimal cache usage, but can still be accessed via a key
	 */
	template<typename ValueT, size_t Size>
	class MappedArray
	{
	public:

		MappedArray()
		{
			mArraySize = 0;
		}

		// Insert new Value into Array
		void Insert(const size_t id, const ValueT& value)
		{
			assert(mIdToIndex.find(id) == mIdToIndex.end() && "Value with that ID already exists");

			// Insert Value at end of array
			size_t newIndex = mArraySize;
			mIdToIndex[id] = newIndex;
			mIndexToId[newIndex] = id;
			mArray[newIndex] = value;

			mArraySize++;
		}

		// Remove value from Array
		void Erase(const size_t id)
		{
			assert(mIdToIndex.find(id) != mIdToIndex.end() && "Removing non-existent value");

			if (mArraySize == 0 || mIdToIndex.count(id) == 0)
				return;

			// Copy value at end of array into deleted values space to maintain packed array
			size_t indexOfRemovedValue = mIdToIndex[id];
			size_t indexOfLastValue = mArraySize - 1;
			mArray[indexOfRemovedValue] = mArray[indexOfLastValue];

			// Update map to point to values new location
			const size_t idOfLastValue = mIndexToId[indexOfLastValue];
			mIdToIndex[idOfLastValue] = indexOfRemovedValue;
			mIndexToId[indexOfRemovedValue] = idOfLastValue;

			mIdToIndex.erase(id);
			mIdToIndex.erase(indexOfLastValue);

			mArraySize--;
		}

		[[nodiscard]] bool Contains(size_t id) const
		{
			return mIdToIndex.count(id) == 1;
		}

		void Clear()
		{
			mIdToIndex.clear();
			mIdToIndex.clear();
			mArraySize = 0;
		}

		auto begin()
		{
			return mArray.begin();
		}

		auto end()
		{
			return mArray.end();
		}

		const ValueT& operator[](const size_t& id) const
		{
			return mArray[mIdToIndex.at(id)];
		}

		ValueT& operator[](const size_t& id)
		{
			return mArray[mIdToIndex[id]];
		}

	private:

		std::array<ValueT, Size> mArray; // Packed array of types
		size_t mArraySize; // Number of valid entities in array

		std::unordered_map<size_t, size_t> mIdToIndex; // Map from id to index
		std::unordered_map<size_t, size_t> mIndexToId; // Map from index to id
	};

	// Custom bitset that ensures in use bits are packed together
	template<size_t Size>
	class PackedBitset
	{
	public:

		PackedBitset()
		{
			mBitsetSize = 0;
		}

		void Insert(const size_t id, const bool& value = false)
		{
			assert(mIdToIndex.find(id) == mIdToIndex.end() && "Value with that ID already exists");

			// Insert value at end of bitset
			size_t newIndex = mBitsetSize;
			mIdToIndex[id] = newIndex;
			mIndexToId[newIndex] = id;
			mBitset[newIndex] = value;

			mBitsetSize++;
		}

		// Remove value from bitset
		void Erase(const size_t id)
		{
			assert(mIdToIndex.find(id) != mIdToIndex.end() && "Removing non-existent value");

			if (mBitsetSize == 0 || mIdToIndex.count(id) == 0)
				return;

			// Copy value at end of array into deleted values space to maintain packed array
			size_t indexOfRemovedValue = mIdToIndex[id];
			size_t indexOfLastValue = mBitsetSize - 1;
			mBitset[indexOfRemovedValue] = mBitset[indexOfLastValue];

			// Update map to point to values new location
			const size_t idOfLastValue = mIndexToId[indexOfLastValue];
			mIdToIndex[idOfLastValue] = indexOfRemovedValue;
			mIndexToId[indexOfRemovedValue] = idOfLastValue;

			mIdToIndex.erase(id);
			mIndexToId.erase(indexOfLastValue);

			mBitsetSize--;
		}

		[[nodiscard]] bool Contains(const size_t id) const
		{
			return mIdToIndex.count(id) == 1;
		}

		void Clear()
		{
			mIdToIndex.clear();
			mIdToIndex.clear();
			mBitsetSize = 0;
		}

		typename std::bitset<Size>::reference operator[](const size_t id)
		{
			return mBitset[mIdToIndex[id]];
		}

	private:

		std::bitset<Size> mBitset; // Internal Bitset
		size_t mBitsetSize; // Number of bits currently occupied in bitset

		std::unordered_map<size_t, size_t> mIdToIndex; // Map from id to index
		std::unordered_map<size_t, size_t> mIndexToId; // Map from index to id
	};
}