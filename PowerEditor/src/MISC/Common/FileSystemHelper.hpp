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
#include <string>
#include <vector>
#include <tuple>
#include <optional>
#include <functional>
#include <string_view>

#include <Windows.h>

#include "UniqueBuffer.hpp"

#include "MathHelper.hpp"

class FileSystemHelper
{
public:
	FileSystemHelper() noexcept = delete;

	enum class PrepareAlignedBufferForUnbufferedAccessResult : uint32_t
	{
		SUCCESS = 0,
		FAILED_GET_FILE_SIZE = 100,
		FAILED_BUFFER_SIZE_BIGGER_THAN_SIZE_TYPE = 101,
		FAILED_BUFFER_ALLOCATION = 102,
		FAILED_GET_STORAGE_INFO = 103,
		FAILED_GIVEN_BUFFER_NOT_ALIGNED_CORRECTLY = 104,
		FAILED_GIVEN_BUFFER_NOT_BIG_ENOUGH = 105,
		FAILED_NO_BUFFER_GIVEN = 106
	};

	enum class ReadFileResult : uint32_t
	{
		SUCCESS = 0,
		FAILED_OPEN_FILE = 1,
		FAILED_READ = 2,

		// + inherits the enum values from PrepareAlignedBufferForUnbufferedAccessResult
		FAILED_GET_FILE_SIZE = 100,
		FAILED_BUFFER_SIZE_BIGGER_THAN_SIZE_TYPE = 101,
		FAILED_BUFFER_ALLOCATION = 102,
		FAILED_GET_STORAGE_INFO = 103,
		FAILED_GIVEN_BUFFER_NOT_ALIGNED_CORRECTLY = 104,
		FAILED_GIVEN_BUFFER_NOT_BIG_ENOUGH = 105,
		FAILED_NO_BUFFER_GIVEN = 106
		// - inherits the enum values from PrepareAlignedBufferForUnbufferedAccessResult
	};

	enum class WriteFileResult : uint32_t
	{
		SUCCESS = 0,
		FAILED_OPEN_FILE = 1,
		FAILED_WRITE = 2,
		FAILED_SET_ALLOCATION_INFO = 3,
		FAILED_SET_END_OF_FILE = 4,

		// + inherits the enum values from PrepareAlignedBufferForUnbufferedAccessResult
		FAILED_GET_FILE_SIZE = 100,
		FAILED_BUFFER_SIZE_BIGGER_THAN_SIZE_TYPE = 101,
		FAILED_BUFFER_ALLOCATION = 102,
		FAILED_GET_STORAGE_INFO = 103,
		FAILED_GIVEN_BUFFER_NOT_ALIGNED_CORRECTLY = 104,
		FAILED_GIVEN_BUFFER_NOT_BIG_ENOUGH = 105,
		FAILED_NO_BUFFER_GIVEN = 106
		// - inherits the enum values from PrepareAlignedBufferForUnbufferedAccessResult
	};

	enum class CreateEmptyFileResult : uint32_t
	{
		SUCCESS = 0,
		FAILED_FILE_ALREADY_EXISTS = 1,
		FAILED_UNABLE_TO_CREATE = 2,
	};

	class UnbufferedFileAccessBuffer : public UniqueBuffer<std::byte, decltype(&::_aligned_free)>
	{
	private:
		using aligned_free_deleter = decltype(&::_aligned_free);
	public:
		static constexpr const size_t TYPICAL_PHYSICAL_SECTOR_ALIGNMENT = 4 * 1024;

		UnbufferedFileAccessBuffer() noexcept
			: UniqueBuffer<std::byte, aligned_free_deleter>{nullptr}
		{}

		UnbufferedFileAccessBuffer(size_t requiredSize, size_t physicalSectorAlignment) noexcept
			: UniqueBuffer<std::byte, aligned_free_deleter>{reinterpret_cast<std::byte*>(::_aligned_malloc(MathHelper::roundUpToPowerOf2(requiredSize, physicalSectorAlignment), physicalSectorAlignment)),
			MathHelper::roundUpToPowerOf2(requiredSize, physicalSectorAlignment), &::_aligned_free}
		{}
	};

	static CreateEmptyFileResult createEmptyFile(const std::wstring &filePath) noexcept;

	// if this function fails with FAILED_OPEN_FILE you can call GetLastError to get the real reason for that
	[[nodiscard]]
	static std::tuple<ReadFileResult, size_t, std::optional<UnbufferedFileAccessBuffer>> readFileContentUnbuffered(const std::wstring &filePath, BufferInfo<std::byte> readToBuffer, bool allowCreateNewBuffer = true) noexcept;

	// if this function fails with FAILED_OPEN_FILE you can call GetLastError to get the real reason for that
	static WriteFileResult writeFileContentUnbuffered(const std::wstring &filePath, const BufferInfo<std::byte> data, size_t actualDataSize, bool allowOverwrite = true, bool preallocate = true, bool writeThrough = true, bool allowCreateNewBuffer = true) noexcept;

	// convenience alternative
	template<typename CharT, typename Traits = std::char_traits<CharT>>
	static WriteFileResult writeFileContentUnbuffered(const std::wstring &filePath, const std::basic_string_view<CharT, Traits> stringView, bool allowOverwrite = true, bool preallocate = true, bool writeThrough = true, bool allowCreateNewBuffer = true) noexcept
	{
		size_t size = stringView.size() * sizeof(CharT);
		const BufferInfo<std::byte> data{const_cast<std::byte*>(reinterpret_cast<const std::byte*>(stringView.data())), size};
		return writeFileContentUnbuffered(filePath, data, size, allowOverwrite, preallocate, writeThrough, allowCreateNewBuffer);
	}

	// returns fileSize and allocationSize
	[[nodiscard]]
	static std::optional<std::tuple<uint64_t, uint64_t>> getFileSize(const std::wstring &filePath) noexcept;

	// the handle does not need any special access
	// returns fileSize and allocationSize
	[[nodiscard]]
	static std::optional<std::tuple<uint64_t, uint64_t>> getFileSize(HANDLE fileHandle) noexcept;

	[[nodiscard]]
	static std::optional<FILE_STORAGE_INFO> getStorageInfo(HANDLE fileHandle) noexcept;

	static bool setEndOfFile(HANDLE fileHandle, uint64_t fileSize) noexcept;
	static bool setFileAllocationSize(HANDLE fileHandle, uint64_t allocationSize) noexcept;

private:
	[[nodiscard]]
	static std::tuple<PrepareAlignedBufferForUnbufferedAccessResult, std::optional<UnbufferedFileAccessBuffer>> checkOrCreateAlignedBufferForUnbufferedAccess(const BufferInfo<std::byte> buffer, uint64_t requiredSize, uint64_t physicalSectorAlignment, bool allowCreateNewBuffer = true) noexcept;
};

