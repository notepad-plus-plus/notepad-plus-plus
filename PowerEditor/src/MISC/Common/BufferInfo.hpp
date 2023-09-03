// This file is part of Notepad++ project
// Copyright (C)2023 Klotzi111

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <stdint.h>
#include <memory>
#include <utility>
#include <concepts>
#include <type_traits>
#include <cassert>
#include <stdexcept>
#include <iterator>
#include <vector>

template<typename T1, typename T2>
concept not_same_as = !std::same_as<T1, T2>;

template<typename TIterator>
concept random_access_iterator_or_value_type_is_void = std::same_as<typename TIterator::value_type, void> || std::random_access_iterator<TIterator>;

namespace __detail
{
template<typename ValueType, typename PointerType, typename ReferenceType>
	requires (!std::is_reference_v<ValueType>) && (std::same_as<std::remove_cv_t<ValueType>, void> || (std::is_pointer_v<PointerType> && std::is_reference_v<ReferenceType>))
		&& std::same_as<std::remove_pointer_t<PointerType>, ValueType> && std::same_as<std::remove_reference_t<ReferenceType>, ValueType>
class iterator_impl
{
	ValueType* _ptr;
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = ValueType;
    using difference_type = std::ptrdiff_t;
    using pointer = PointerType;
    using reference = ReferenceType;

    explicit iterator_impl(ValueType* ptr = nullptr) noexcept
    	: _ptr(ptr)
    {}

    constexpr bool operator==(const iterator_impl& other) const noexcept { return _ptr == other._ptr; }
    constexpr bool operator!=(const iterator_impl& other) const noexcept { return _ptr != other._ptr; }

    // for weakly_incrementable
    constexpr iterator_impl& operator++() noexcept { ++_ptr; return *this; }
    constexpr iterator_impl operator++(int) noexcept { iterator_impl tmp(*this); ++(*this); return tmp; }

    // for input_or_output_iterator
    constexpr reference operator*() const noexcept { return *_ptr; }

    constexpr pointer operator->() const noexcept { return _ptr; }

    // for bidirectional_iterator
    constexpr iterator_impl& operator--() noexcept { --_ptr; return *this; }
    constexpr iterator_impl operator--(int) noexcept { iterator_impl tmp(*this); --(*this); return tmp; }

    // for random_access_iterator
    constexpr iterator_impl& operator+=(const difference_type other) noexcept { _ptr += other; return *this; }
    constexpr iterator_impl& operator-=(const difference_type other) noexcept { _ptr -= other; return *this; }
    constexpr iterator_impl operator+(const difference_type other) const noexcept { return iterator_impl(_ptr + other); }
    constexpr iterator_impl operator-(const difference_type other) const noexcept { return iterator_impl(_ptr - other); }
    constexpr iterator_impl operator+(const iterator_impl& other) const noexcept { return *this + other._ptr; }
    constexpr difference_type operator-(const iterator_impl& other) const noexcept { return difference_type(_ptr) - difference_type(other._ptr); }
    constexpr reference operator[](const difference_type index) const { return _ptr[index]; }

    constexpr reference operator[](const size_t index) const { return _ptr[index]; }

    constexpr bool operator<(const iterator_impl& other) const noexcept { return _ptr < other._ptr; }
    constexpr bool operator>(const iterator_impl& other) const noexcept { return _ptr > other._ptr; }
    constexpr bool operator<=(const iterator_impl& other) const noexcept { return _ptr <= other._ptr; }
    constexpr bool operator>=(const iterator_impl& other) const noexcept { return _ptr >= other._ptr; }
};

template<typename ValueType, typename PointerType, typename ReferenceType>
constexpr iterator_impl<ValueType, PointerType, ReferenceType> operator+(const typename iterator_impl<ValueType, PointerType, ReferenceType>::difference_type diff, const iterator_impl<ValueType, PointerType, ReferenceType> iter) noexcept
{
	return iter + diff;
}

template<typename T>
class BufferAccessor
{
public:
	using pointer_type = T*;
	using const_pointer_type = const T*;
	using element_type = T;

	using reference_type = typename std::add_lvalue_reference_t<std::remove_reference_t<T>>;
	using const_reference_type = typename std::add_lvalue_reference_t<const std::remove_reference_t<T>>;

	using iterator = iterator_impl<element_type, pointer_type, reference_type>;
	using const_iterator = iterator_impl<const element_type, const_pointer_type, const_reference_type>;
	static_assert(random_access_iterator_or_value_type_is_void<iterator>, "element_type must be void or iterator must satisfy random_access_iterator concept");

public:
	[[nodiscard]]
	virtual constexpr pointer_type data() noexcept = 0;

	[[nodiscard]]
	virtual constexpr const_pointer_type data() const noexcept = 0;

	[[nodiscard]]
	virtual constexpr size_t rawBufferSize() const noexcept = 0;

	[[nodiscard]]
	explicit operator bool() const noexcept
	{
		return data() != nullptr;
	}

	template<not_same_as<void> _TempT = element_type>
	[[nodiscard]]
	constexpr size_t size() const noexcept
	{
		return rawBufferSize() / sizeof(element_type);
	}

	template<std::same_as<void> _TempT = element_type>
	[[nodiscard]]
	constexpr size_t size() const noexcept
	{
		return rawBufferSize();
	}

	// + iterators
	// ++ forward
	template<not_same_as<void> _TempT = element_type>
	[[nodiscard]]
	constexpr iterator begin() noexcept
	{
		return iterator(data());
	}

	template<not_same_as<void> _TempT = element_type>
	[[nodiscard]]
	constexpr const_iterator begin() const noexcept
	{
		return const_iterator(data());
	}

	template<not_same_as<void> _TempT = element_type>
	[[nodiscard]]
	constexpr const_iterator cbegin() const noexcept
	{
		return const_iterator(data());
	}

	template<not_same_as<void> _TempT = element_type>
	[[nodiscard]]
	constexpr iterator end() noexcept
	{
		return iterator(data() + size());
	}

	template<not_same_as<void> _TempT = element_type>
	[[nodiscard]]
	constexpr const_iterator end() const noexcept
	{
		return const_iterator(data() + size());
	}

	template<not_same_as<void> _TempT = element_type>
	[[nodiscard]]
	constexpr const_iterator cend() const noexcept
	{
		return const_iterator(data() + size());
	}
	// - forward

	// + reverse
	template<not_same_as<void> _TempT = element_type>
	[[nodiscard]]
	constexpr auto rbegin() noexcept
	{
		return std::make_reverse_iterator(end());
	}

	template<not_same_as<void> _TempT = element_type>
	[[nodiscard]]
	constexpr auto rbegin() const noexcept
	{
		return std::make_reverse_iterator(end());
	}

	template<not_same_as<void> _TempT = element_type>
	[[nodiscard]]
	constexpr auto crbegin() const noexcept
	{
		return std::make_reverse_iterator(cend());
	}

	template<not_same_as<void> _TempT = element_type>
	[[nodiscard]]
	constexpr auto rend() noexcept
	{
		return std::make_reverse_iterator(begin());
	}

	template<not_same_as<void> _TempT = element_type>
	[[nodiscard]]
	constexpr auto rend() const noexcept
	{
		return std::make_reverse_iterator(begin());
	}

	template<not_same_as<void> _TempT = element_type>
	[[nodiscard]]
	constexpr auto crend() const noexcept
	{
		return std::make_reverse_iterator(cbegin());
	}
	// - reverse
    // - interators

	template<not_same_as<void> _TempT = element_type>
	[[nodiscard]]
	constexpr const_reference_type at(size_t pos) const
	{
		checkValidPosition(pos);
		return data()[pos];
	}

	template<not_same_as<void> _TempT = element_type>
	[[nodiscard]]
	constexpr reference_type at(size_t pos)
	{
		checkValidPosition(pos);
		return data()[pos];
	}

	template<not_same_as<void> _TempT = element_type>
	[[nodiscard]]
	constexpr const_reference_type operator[](size_t pos) const noexcept
	{
		// assert Access out of bounds
		assert(pos < size());
		return data()[pos];
	}

	template<not_same_as<void> _TempT = element_type>
	[[nodiscard]]
	constexpr reference_type operator[](size_t pos) noexcept
	{
		// assert Access out of bounds
		assert(pos < size());
		return data()[pos];
	}

private:
	constexpr void checkValidPosition(size_t pos) const
	{
		if (pos >= size())
		{
			throw std::out_of_range{"pos >= size()"};
		}
	}
};
}

// This class is very similar to std::span
template<typename T>
class BufferInfo : public __detail::BufferAccessor<T>
{
private:
	T* _bufferPtr;
	size_t _bufferSize;

	using buffer_accessor_type = __detail::BufferAccessor<T>;

	void fixSizeForNullptr()
	{
		if (_bufferPtr == nullptr)
		{
			_bufferSize = 0;
		}
	}
public:
	using pointer_type = typename buffer_accessor_type::pointer_type;
	using const_pointer_type = typename buffer_accessor_type::const_pointer_type;
	using element_type = typename buffer_accessor_type::element_type;

	BufferInfo() noexcept
		: _bufferPtr{nullptr}, _bufferSize{0}
	{}

	template<not_same_as<void> _TempT = element_type>
	BufferInfo(pointer_type bufferPtr, size_t bufferElementCount) noexcept
		: _bufferPtr{bufferPtr}, _bufferSize{bufferElementCount * sizeof(element_type)}
	{
		fixSizeForNullptr();
	}

	template<std::same_as<void> _TempT = element_type>
	BufferInfo(pointer_type bufferPtr, size_t bufferSize) noexcept
		: _bufferPtr{bufferPtr}, _bufferSize{bufferSize}
	{
		fixSizeForNullptr();
	}

	BufferInfo(std::vector<element_type> &vector) noexcept
		: BufferInfo{vector.data(), vector.size()}
	{
		fixSizeForNullptr();
	}

	[[nodiscard]]
	virtual constexpr pointer_type data() noexcept override
	{
		return _bufferPtr;
	}

	[[nodiscard]]
	virtual constexpr const_pointer_type data() const noexcept override
	{
		return _bufferPtr;
	}

	[[nodiscard]]
	virtual constexpr size_t rawBufferSize() const noexcept override
	{
		return _bufferSize;
	}

};
