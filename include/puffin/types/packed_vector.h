#pragma once

#include <cassert>
#include <vector>
#include <unordered_map>

namespace puffin
{
	template<typename KeyT, typename ValueT>
	class PackedVector
	{
	public:

		void clear()
		{
			m_vector.clear();
			m_key_to_idx.clear();
			m_idx_to_key.clear();
			m_count = 0;
		}

		bool emplace(const KeyT& key, const ValueT& value)
		{
			if (contains(key))
				return false;

			if (m_count >= m_vector.size())
			{
				m_vector.push_back(value);
			}
			else
			{
				m_vector[m_count] = value;
			}
			
			m_key_to_idx.emplace(key, m_count);
			m_idx_to_key.emplace(m_count, key);

			m_count++;

			return true;
		}

		// Erase an element from vector
		// should_internal_vector_shrink - Whether the internal vector should shrink in size
		bool erase(const KeyT& key, bool should_internal_vector_shrink = true)
		{
			if (!contains(key))
				return false;

			// Swap erased and last elements to maintain contiguous memory
			auto removed_value_idx = m_key_to_idx[key];
			auto last_value_idx = m_count - 1;

			swap_by_idx(removed_value_idx, last_value_idx);

			if (should_internal_vector_shrink)
			{
				// Delete element at end of vector
				m_vector.pop_back();
			}

			// Remove old map values
			m_key_to_idx.erase(key);
			m_idx_to_key.erase(last_value_idx);

			--m_count;

			return true;
		}

		void pop_back(bool should_internal_vector_shrink = true)
		{
			auto last_value_idx = m_count - 1;
			auto last_value_key = m_idx_to_key.at(last_value_idx);

			m_idx_to_key.erase(last_value_idx);
			m_key_to_idx.erase(last_value_key);

			if (should_internal_vector_shrink)
			{
				// Delete element at end of vector
				m_vector.pop_back();
			}

			--m_count;
		}

		bool contains(const KeyT& key) const
		{
			return m_key_to_idx.find(key) != m_key_to_idx.end();
		}

		ValueT& at(const KeyT& key)
		{
			assert(contains(key) && "PackedVector::at - Vector does not contain an element with this key");

			return m_vector.at(m_key_to_idx.at(key));
		}

		const ValueT& at(const KeyT& key) const
		{
			return m_vector.at(m_key_to_idx.at(key));
		}

		// Resize internal vector, does not change the value return by size(), use size_internal() instead
		void resize(const size_t new_size)
		{
			m_vector.resize(new_size);
		}

		// Shrink internal vector size to match count
		void shrink_to_count()
		{
			m_vector.resize(m_count);
		}

		// Reserve space in internal vector
		void reserve(const size_t new_size)
		{
			m_vector.reserve(new_size);
		}

		// Shrink internal vector capacity to match size
		void shrink_to_fit()
		{
			m_vector.resize(m_count);
		}

		// Return number of valid elements (i.e, elements with an associated key)
		[[nodiscard]] const size_t count() const
		{
			return m_count;
		}

		// Return number of elements in internal vector
		[[nodiscard]] const size_t size() const
		{
			return m_vector.size();
		}

		// Returns number of elements internal vector has space allocated for
		[[nodiscard]] const size_t capacity() const
		{
			return m_vector.capacity();
		}

		auto begin()
		{
			return m_vector.begin();
		}

		auto end()
		{
			return m_vector.begin() + m_count;
		}

		const auto begin() const
		{
			return m_vector.begin();
		}

		const auto end() const
		{
			return m_vector.begin() + m_count;
		}

		auto rbegin()
		{
			return m_vector.rbegin();
		}

		auto rend()
		{
			return m_vector.rbegin() + m_count;
		}

		auto cbegin() const
		{
			return m_vector.cbegin();
		}

		auto cend() const
		{
			return m_vector.cend() + m_count;
		}

		auto crbegin() const
		{
			return m_vector.crbegin();
		}

		auto crend() const
		{
			return m_vector.crend() + m_count;
		}

		void sort()
		{
			sort_bubble();
		}

		ValueT* data()
		{
			return m_vector.data();
		}

		const ValueT* data() const
		{
			return m_vector.data();
		}

		ValueT& operator[](const KeyT& key)
		{
			//emplace(key, ValueT());

			return at(key);
		}

		const ValueT& operator[](const KeyT& key) const
		{
			assert(contains(key) && "PackedVector::operator[] - Vector does not contain an element with this key");

			return at(key);
		}

		ValueT& idx(const size_t& idx)
		{
			return m_vector.at(idx);
		}

		const ValueT& idx(const size_t& idx) const
		{
			return m_vector.at(idx);
		}

	private:

		size_t m_count = 0; // Number of valid elements in array
		std::vector<ValueT> m_vector;
		std::unordered_map<KeyT, size_t> m_key_to_idx;
		std::unordered_map<size_t, KeyT> m_idx_to_key;

		void swap_by_idx(const size_t& idx_a, const size_t& idx_b)
		{
			auto key_a = m_idx_to_key.at(idx_a);

			// Swap values
			ValueT value_a = m_vector.at(idx_a);

			m_vector.at(idx_a) = m_vector.at(idx_b);
			m_vector.at(idx_b) = value_a;

			// Update key map to match swapped values
			m_idx_to_key.at(idx_a) = m_idx_to_key.at(idx_b);
			m_idx_to_key.at(idx_b) = key_a;

			// Update idx map to match swapped values
			m_key_to_idx.at(m_idx_to_key.at(idx_a)) = idx_b;
			m_key_to_idx.at(m_idx_to_key.at(idx_b)) = idx_a;
		}

		void swap_by_key(const KeyT& key_a, const KeyT& key_b)
		{
			auto idx_a = m_key_to_idx.at(key_a);
			auto idx_b = m_key_to_idx.at(key_b);

			// Swap values
			auto value_a = m_vector.at(idx_a);

			m_vector.at(idx_a) = m_vector.at(idx_b);
			m_vector.at(idx_b) = value_a;

			// Update key map to match swapped values
			m_idx_to_key.at(idx_a) = key_b;
			m_idx_to_key.at(idx_b) = key_a;

			// Update idx map to match swapped values
			m_key_to_idx.at(key_a) = idx_b;
			m_key_to_idx.at(key_b) = idx_a;
		}

		void sort_bubble()
		{
			for (size_t i = 0; i < m_count - 1; ++i)
			{
				bool swapped = false;

				for (size_t j = 0; j < m_count - i - 1; ++j)
				{
					if (m_vector.at(j + 1) < m_vector.at(j))
					{
						swap_by_idx(j, j + 1);
						swapped = true;
					}
				}

				if (!swapped)
					break;
			}
		}

	};
}