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

#include "BufferInfo.hpp"

template<typename T>
concept unique_ptr_non_function_deleter = !std::is_pointer_v<T> && std::is_default_constructible_v<T>;

template<typename T>
concept unique_ptr_function_deleter = std::is_copy_constructible_v<T>;

template<typename T, typename Deleter = std::default_delete<T>>
class UniqueBuffer : public __detail::BufferAccessor<T>
{
private:
	std::unique_ptr<T, Deleter> _uniquePtr;
	size_t _bufferSize;

	using buffer_accessor_type = __detail::BufferAccessor<T>;

	void fixSizeForNullptr()
	{
		if (!_uniquePtr)
		{
			_bufferSize = 0;
		}
	}
public:
	using deleter_type = Deleter;

	using pointer_type = typename buffer_accessor_type::pointer_type;
	using const_pointer_type = typename buffer_accessor_type::const_pointer_type;
	using element_type = typename buffer_accessor_type::element_type;

	template<unique_ptr_non_function_deleter DeleterFunction = deleter_type>
	UniqueBuffer() noexcept
		: _uniquePtr{nullptr}, _bufferSize{0}
	{}

	template<unique_ptr_non_function_deleter DeleterFunction = deleter_type>
		requires not_same_as<element_type, void>
	UniqueBuffer(pointer_type bufferPtr, size_t bufferElementCount) noexcept
		: _uniquePtr{bufferPtr}, _bufferSize{bufferElementCount * sizeof(element_type)}
	{
		fixSizeForNullptr();
	}

	template<unique_ptr_non_function_deleter DeleterFunction = deleter_type>
		requires std::same_as<element_type, void>
	UniqueBuffer(pointer_type bufferPtr, size_t bufferSize) noexcept
		: _uniquePtr{bufferPtr}, _bufferSize{bufferSize}
	{
		fixSizeForNullptr();
	}

	template<unique_ptr_function_deleter DeleterFunction = deleter_type>
	UniqueBuffer(const Deleter deleter) noexcept
		: _uniquePtr{nullptr, deleter}, _bufferSize{0}
	{}

	template<unique_ptr_function_deleter DeleterFunction = deleter_type>
		requires not_same_as<element_type, void>
	UniqueBuffer(pointer_type bufferPtr, size_t bufferElementCount, const Deleter deleter) noexcept
		: _uniquePtr{bufferPtr, deleter}, _bufferSize{bufferElementCount * sizeof(element_type)}
	{
		fixSizeForNullptr();
	}

	template<unique_ptr_function_deleter DeleterFunction = deleter_type>
		requires std::same_as<element_type, void>
	UniqueBuffer(pointer_type bufferPtr, size_t bufferSize, const Deleter deleter) noexcept
		: _uniquePtr{bufferPtr, deleter}, _bufferSize{bufferSize}
	{
		fixSizeForNullptr();
	}

	UniqueBuffer(const UniqueBuffer& other) noexcept = delete;
	UniqueBuffer& operator=(const UniqueBuffer& other) noexcept = delete;

	UniqueBuffer(UniqueBuffer&& other) noexcept
		: _uniquePtr(std::move(other._uniquePtr)), _bufferSize(std::exchange(other._bufferSize, 0))
	{}

	UniqueBuffer& operator=(UniqueBuffer&& other) noexcept
	{
		if (this != &other)
		{
			_uniquePtr = std::move(other._uniquePtr);
			_bufferSize = std::exchange(other._bufferSize, 0);
		}
		return *this;
	}

	~UniqueBuffer() noexcept
	{
		_bufferSize = 0;
	}

	[[nodiscard]]
	virtual constexpr pointer_type data() noexcept override
	{
		return _uniquePtr.get();
	}

	[[nodiscard]]
	virtual constexpr const_pointer_type data() const noexcept override
	{
		return _uniquePtr.get();
	}

	[[nodiscard]]
	virtual constexpr size_t rawBufferSize() const noexcept override
	{
		return _bufferSize;
	}

};
