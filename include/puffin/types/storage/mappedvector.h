#pragma once

#include <cassert>
#include <vector>
#include <unordered_map>

namespace puffin
{
	template<typename KeyT, typename ValueT>
	class MappedVector
	{
	public:

		void Emplace(const KeyT& key, const ValueT& value)
		{
			if (mCount >= mData.size())
			{
				mData.push_back(value);
			}
			else
			{
				mData[mCount] = value;
			}
			
			mKeyToIdx.emplace(key, mCount);
			mIdxToKey.emplace(mCount, key);

			++mCount;
		}

		// Erase an element from vector
		// should_internal_vector_shrink - Whether the internal vector should shrink in size
		void Erase(const KeyT& key, const bool shouldInternalVectorShrink = true)
		{
			if (!Contains(key))
				return;

			// Swap erased and last elements to maintain contiguous memory
			auto removedValueIdx = mKeyToIdx[key];
			auto lastValueIdx = mCount - 1;

			SwapByIdx(removedValueIdx, lastValueIdx);

			if (shouldInternalVectorShrink)
			{
				// Delete element at end of vector
				mData.pop_back();
			}

			// Remove old map values
			mKeyToIdx.erase(key);
			mIdxToKey.erase(lastValueIdx);

			--mCount;
		}

		void PopBack(const bool shouldInternalVectorShrink = true)
		{
			auto lastValueIdx = mCount - 1;
			auto lastValueKey = mIdxToKey.at(lastValueIdx);

			mIdxToKey.erase(lastValueIdx);
			mKeyToIdx.erase(lastValueKey);

			if (shouldInternalVectorShrink)
			{
				// Delete element at end of vector
				mData.pop_back();
			}

			--mCount;
		}

		void Clear(bool shouldInternalVectorShrink = true)
		{
			mKeyToIdx.clear();
			mIdxToKey.clear();
			mCount = 0;

			if (shouldInternalVectorShrink)
				mData.clear();
		}

		[[nodiscard]] bool Contains(const KeyT& key) const
		{
			return mKeyToIdx.find(key) != mKeyToIdx.end();
		}

		ValueT& At(const KeyT& key)
		{
			assert(Contains(key) && "MappedVector::At - Vector does not contain an element with this key");

			return mData.at(mKeyToIdx.at(key));
		}

		[[nodiscard]] const ValueT& At(const KeyT& key) const
		{
			return mData.at(mKeyToIdx.at(key));
		}

		// Resize internal vector, does not change the value return by size(), use size_internal() instead
		void Resize(const size_t newSize)
		{
			mData.resize(newSize);
		}

		// Reserve space in internal vector
		void Reserve(const size_t newSize)
		{
			mData.reserve(newSize);
		}

		// Shrink internal vector capacity to match size
		void ShrinkToFit()
		{
			mData.resize(mCount);
		}

		// Return number of valid elements (i.e, elements with an associated key)
		[[nodiscard]] size_t Count() const
		{
			return mCount;
		}

		// Return number of elements in internal vector
		[[nodiscard]] size_t Size() const
		{
			return mData.size();
		}

		// Returns number of elements internal vector has space allocated for
		[[nodiscard]] size_t Capacity() const
		{
			return mData.capacity();
		}

		[[nodiscard]] bool Empty() const
		{
			return mCount == 0;
		}

		[[nodiscard]] bool Full() const
		{
			return mCount == mData.size();
		}

		auto begin()
		{
			return mData.begin();
		}

		auto end()
		{
			return mData.begin() + mCount;
		}

		const auto begin() const
		{
			return mData.begin();
		}

		const auto end() const
		{
			return mData.begin() + mCount;
		}

		auto rbegin()
		{
			return mData.rbegin();
		}

		auto rend()
		{
			return mData.rbegin() + mCount;
		}

		auto cbegin() const
		{
			return mData.cbegin();
		}

		auto cend() const
		{
			return mData.cend() + mCount;
		}

		auto crbegin() const
		{
			return mData.crbegin();
		}

		auto crend() const
		{
			return mData.crend() + mCount;
		}

		void Sort()
		{
			SortBubble();
		}

		ValueT* Data()
		{
			return mData.data();
		}

		const ValueT* Data() const
		{
			return mData.data();
		}

		ValueT& operator[](const KeyT& key)
		{
			assert(Contains(key) && "MappedVector::operator[] - Vector does not contain an element with this key");

			return mData.at(mKeyToIdx.at(key));
		}

		const ValueT& operator[](const KeyT& key) const
		{
			assert(Contains(key) && "MappedVector::operator[] - Vector does not contain an element with this key");

			return mData.at(mKeyToIdx.at(key));
		}

		ValueT& Idx(const size_t& idx)
		{
			return mData.at(idx);
		}

		const ValueT& Idx(const size_t& idx) const
		{
			return mData.at(idx);
		}

	private:

		size_t mCount = 0; // Number of valid elements in array
		std::vector<ValueT> mData;
		std::unordered_map<KeyT, size_t> mKeyToIdx;
		std::unordered_map<size_t, KeyT> mIdxToKey;

		void SwapByIdx(const size_t& idxA, const size_t& idxB)
		{
			auto keyA = mIdxToKey.at(idxA);
			auto keyB = mIdxToKey.at(idxB);

			// Swap values
			ValueT valueA = mData.at(idxA);

			mData.at(idxA) = mData.at(idxB);
			mData.at(idxB) = valueA;

			// Update key map to match swapped values
			mIdxToKey.at(idxA) = keyB;
			mIdxToKey.at(idxB) = keyA;

			// Update idx map to match swapped values
			mKeyToIdx.at(keyA) = idxB;
			mKeyToIdx.at(keyB) = idxA;
		}

		void SwapByKey(const KeyT& keyA, const KeyT& keyB)
		{
			auto idxA = mKeyToIdx.at(keyA);
			auto idxB = mKeyToIdx.at(keyB);

			// Swap values
			auto valueA = mData.at(idxA);

			mData.at(idxA) = mData.at(idxB);
			mData.at(idxB) = valueA;

			// Update key map to match swapped values
			mIdxToKey.at(idxA) = keyB;
			mIdxToKey.at(idxB) = keyA;

			// Update idx map to match swapped values
			mKeyToIdx.at(keyA) = idxB;
			mKeyToIdx.at(keyB) = idxA;
		}

		void SortBubble()
		{
			for (size_t i = 0; i < mCount - 1; ++i)
			{
				bool swapped = false;

				for (size_t j = 0; j < mCount - i - 1; ++j)
				{
					if (mData.at(j + 1) < mData.at(j))
					{
						SwapByIdx(j, j + 1);
						swapped = true;
					}
				}

				if (!swapped)
					break;
			}
		}

	};
}