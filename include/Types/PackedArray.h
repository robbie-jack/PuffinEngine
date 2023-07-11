#pragma once

#include <cassert>
#include <vector>
#include <array>
#include <unordered_map>
#include <bitset>

namespace puffin
{
	// Fixed and Dynamically Sized Arrays which ensure data is packed tightly to maximize cache utilization
	
	// Packed Fixed Array
	template<typename ValueT, size_t Size>
	class PackedArray
	{
	public:

		PackedArray()
		{
			mArraySize = 0;
		}

		// Insert new Value into Array
		void insert(const size_t id, const ValueT& value)
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
		void erase(const size_t id)
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

		bool contains(size_t id) const
		{
			return mIdToIndex.count(id) == 1;
		}

		void clear()
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

	// Packed Dynamic Array
	template<typename ValueT>
	class PackedVector
	{
	public:

		PackedVector()
		{
			mVectorSize = 0;
		}

		// Initialize Packed Vector with number of items to reserve space for
		explicit PackedVector(size_t initialSize) : PackedVector()
		{
			mVector.reserve(initialSize);
		}

		// Insert new Value into Array
		void insert(const size_t id, const ValueT& value)
		{
			assert(mIdToIndex.find(id) == mIdToIndex.end() && "Value with that ID already exists");

			// Insert Value at end of array
			const size_t newIndex = mVectorSize;
			mIdToIndex[id] = newIndex;
			mIndexToId[newIndex] = id;
			mVector.push_back(value);

			mVectorSize++;
		}

		void insert(const size_t id)
		{
			assert(mIdToIndex.find(id) == mIdToIndex.end() && "Value with that ID already exists");

			// Insert Value at end of array
			const size_t newIndex = mVectorSize;
			mIdToIndex[id] = newIndex;
			mIndexToId[newIndex] = id;

			mVectorSize++;
		}

		void emplace(const size_t id, const ValueT& value)
		{
			assert(mIdToIndex.find(id) == mIdToIndex.end() && "Value with that ID already exists");

			// Insert Value at end of array
			const size_t newIndex = mVectorSize;
			mIdToIndex[id] = newIndex;
			mIndexToId[newIndex] = id;
			mVector.emplace_back(value);

			mVectorSize++;
		}

		// Remove value from Array
		void erase(const size_t id)
		{
			assert(mIdToIndex.find(id) != mIdToIndex.end() && "Removing non-existent value");

			if (mVectorSize == 0 || mIdToIndex.count(id) == 0)
				return;

			// Copy value at end of array into deleted values space to maintain packed array
			size_t indexOfRemovedValue = mIdToIndex[id];
			size_t indexOfLastValue = mVectorSize - 1;
			mVector[indexOfRemovedValue] = mVector[indexOfLastValue];

			// Delete value at end of vector
			mVector.pop_back();

			// Update map to point to values new location
			const size_t idOfLastValue = mIndexToId[indexOfLastValue];
			mIdToIndex[idOfLastValue] = indexOfRemovedValue;
			mIndexToId[indexOfRemovedValue] = idOfLastValue;

			mIdToIndex.erase(id);
			mIndexToId.erase(indexOfLastValue);

			mVectorSize--;
		}

		bool contains(const size_t id) const
		{
			return mIdToIndex.find(id) != mIdToIndex.end();
		}

		void clear()
		{
			mIdToIndex.clear();
			mIdToIndex.clear();
			mVector.clear();
			mVectorSize = 0;
		}

		void reserve(size_t size)
		{
			mVector.reserve(size);
		}

		void resize(size_t size)
		{
			mVector.resize(size);
		}

		size_t size() const
		{
			return mVector.size();
		}

		auto begin()
		{
			return mVector.begin();
		}

		auto end()
		{
			return mVector.end();
		}

		const ValueT& operator[](const size_t& id) const
		{

			return mVector[mIdToIndex.at(id)];
		}

		ValueT& operator[](const size_t& id)
		{
			return mVector[mIdToIndex.at(id)];
<<<<<<< HEAD
		}

		ValueT* data() { return mVector.data(); }

		// Sort internal array using bubble sort (slow for large data sets)
		void sortBubble()
		{
			for (size_t i = 0; i < mVectorSize - 1; ++i)
			{
				bool swapped = false;

				for (size_t j = 0; j < mVectorSize - i - 1; ++j)
				{
					if (mVector[j + 1].operator<(mVector[j]))
					{
						swap(j, j + 1);
						swapped = true;
					}
				}

				if (!swapped)
					break;
			}
=======
>>>>>>> vulkan-renderer
		}

	private:
		
		std::vector<ValueT> mVector; // Packed vector of types
		size_t mVectorSize; // Number of valid entities in vector

		std::unordered_map<size_t, size_t> mIdToIndex; // Map from id to index
		std::unordered_map<size_t, size_t> mIndexToId; // Map from index to id

		void swap(size_t idxA, size_t idxB)
		{
			// Swap values
			ValueT valueA = mVector[idxA];

			mVector[idxA] = mVector[idxB];
			mVector[idxB] = valueA;

			// Update id map to match swapped values
			const size_t idOfValueA = mIndexToId[idxA];

			mIndexToId[idxA] = mIndexToId[idxB];
			mIndexToId[idxB] = idOfValueA;

			// Update index map to match swapped values
			mIdToIndex[mIndexToId[idxA]] = idxB;
			mIdToIndex[mIndexToId[idxB]] = idxA;
		}
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

		void insert(const size_t id, const bool& value = false)
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
		void erase(const size_t id)
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

		bool contains(const size_t id) const
		{
			return mIdToIndex.count(id) == 1;
		}

		void clear()
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