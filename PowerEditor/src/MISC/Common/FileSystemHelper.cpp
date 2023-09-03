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

#include "FileSystemHelper.hpp"

#include <Windows.h>
#include <limits>
#include <cstring>
#include <stdlib.h>

#include "MathHelper.hpp"

using unique_ptr_file_handle = std::unique_ptr<std::remove_pointer_t<HANDLE>, decltype(&::CloseHandle)>;

static constexpr const DWORD FILE_OPEN_SHARE_FLAGS = FILE_SHARE_READ | FILE_SHARE_WRITE;
static constexpr const uint64_t FILE_MAX_BYTES_PER_ACCESS_FUNCTION = static_cast<uint64_t>(std::numeric_limits<DWORD>::max());

std::tuple<FileSystemHelper::PrepareAlignedBufferForUnbufferedAccessResult, std::optional<FileSystemHelper::UnbufferedFileAccessBuffer>> FileSystemHelper::checkOrCreateAlignedBufferForUnbufferedAccess
	(const BufferInfo<std::byte> buffer, uint64_t requiredSize, uint64_t physicalSectorAlignment, bool allowCreateNewBuffer) noexcept
{
	assert(requiredSize > 0);

	uint64_t alignedSize = MathHelper::roundUpToPowerOf2(requiredSize, physicalSectorAlignment);
	if (alignedSize > std::numeric_limits<size_t>::max())
	{
		// not possible (i.e. running in 32 bit mode)
		return {PrepareAlignedBufferForUnbufferedAccessResult::FAILED_BUFFER_SIZE_BIGGER_THAN_SIZE_TYPE, std::optional<FileSystemHelper::UnbufferedFileAccessBuffer>{}};
	}

	bool newBufferRequired = false;
	if (buffer.data() != nullptr)
	{
		uint64_t bufferAddress = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(buffer.data()));
		uint64_t alignedBufferAddress = MathHelper::roundUpToPowerOf2(bufferAddress, physicalSectorAlignment);
		if (bufferAddress != alignedBufferAddress)
		{
			newBufferRequired = true;
			if (!allowCreateNewBuffer)
			{
				return {PrepareAlignedBufferForUnbufferedAccessResult::FAILED_GIVEN_BUFFER_NOT_ALIGNED_CORRECTLY, std::optional<FileSystemHelper::UnbufferedFileAccessBuffer>{}};
			}
		}
		if (buffer.size() < alignedSize)
		{
			newBufferRequired = true;
			if (!allowCreateNewBuffer)
			{
				return {PrepareAlignedBufferForUnbufferedAccessResult::FAILED_GIVEN_BUFFER_NOT_BIG_ENOUGH, std::optional<FileSystemHelper::UnbufferedFileAccessBuffer>{}};
			}
		}
	}
	else
	{
		newBufferRequired = true;
		if (!allowCreateNewBuffer)
		{
			return {PrepareAlignedBufferForUnbufferedAccessResult::FAILED_NO_BUFFER_GIVEN, std::optional<FileSystemHelper::UnbufferedFileAccessBuffer>{}};
		}
	}

	if (!newBufferRequired)
	{
		// return SUCCESS with empty optional. This means the given buffer is suitable
		return {PrepareAlignedBufferForUnbufferedAccessResult::SUCCESS, std::optional<FileSystemHelper::UnbufferedFileAccessBuffer>{}};
	}

	UnbufferedFileAccessBuffer newBuffer{static_cast<size_t>(requiredSize), static_cast<size_t>(physicalSectorAlignment)};
	if (!newBuffer)
	{
		return {PrepareAlignedBufferForUnbufferedAccessResult::FAILED_BUFFER_ALLOCATION, std::optional<FileSystemHelper::UnbufferedFileAccessBuffer>{}};
	}

	return {PrepareAlignedBufferForUnbufferedAccessResult::SUCCESS, std::optional<FileSystemHelper::UnbufferedFileAccessBuffer>{std::move(newBuffer)}};
}

FileSystemHelper::CreateEmptyFileResult FileSystemHelper::createEmptyFile(const std::wstring &filePath) noexcept
{
	unique_ptr_file_handle fileHandle{::CreateFileW(filePath.c_str(), GENERIC_WRITE, FILE_OPEN_SHARE_FLAGS, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr), &::CloseHandle};
	if (fileHandle.get() != INVALID_HANDLE_VALUE)
	{
		auto lastError = ::GetLastError();
		if (lastError == ERROR_ALREADY_EXISTS)
		{
			return CreateEmptyFileResult::FAILED_FILE_ALREADY_EXISTS;
		}
		else if (lastError == ERROR_SUCCESS)
		{
			return CreateEmptyFileResult::SUCCESS;
		}
	}

	return CreateEmptyFileResult::FAILED_UNABLE_TO_CREATE;
}

std::tuple<FileSystemHelper::ReadFileResult, size_t, std::optional<FileSystemHelper::UnbufferedFileAccessBuffer>> FileSystemHelper::readFileContentUnbuffered(const std::wstring &filePath, BufferInfo<std::byte> readToBuffer, bool allowCreateNewBuffer) noexcept
{
	uint64_t totalBytesRead = 0;
	BufferInfo<std::byte> sectorAlignedBuffer = readToBuffer;
	UnbufferedFileAccessBuffer newBuffer;

	unique_ptr_file_handle fileHandle{::CreateFileW(filePath.c_str(), GENERIC_READ, FILE_OPEN_SHARE_FLAGS, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING, nullptr), &::CloseHandle};
	if (fileHandle.get() == INVALID_HANDLE_VALUE)
	{
		return {ReadFileResult::FAILED_OPEN_FILE, 0, std::optional<UnbufferedFileAccessBuffer>{}};
	}

	// + get file size
	auto optionalSize = getFileSize(fileHandle.get());
	if (!optionalSize)
	{
		return {ReadFileResult::FAILED_GET_FILE_SIZE, 0, std::optional<UnbufferedFileAccessBuffer>{}};
	}
	auto [fileSize, _] = optionalSize.value();
	// - get file size

	// + get physicalSectorAlignment
	auto optionalStorageInfo = getStorageInfo(fileHandle.get());
	if (!optionalStorageInfo)
	{
		return {ReadFileResult::FAILED_GET_STORAGE_INFO, 0, std::optional<UnbufferedFileAccessBuffer>{}};
	}
	auto storageInfo = optionalStorageInfo.value();

	// for some reason this value can be "wrong". It might be the logical sector size. So the WriteFile function will still work. But we might loss atomicity for the last sector write
	// these wrong reports can occur with flash drives (maybe because of the filesystem on them?!?)
	uint64_t physicalSectorAlignment = storageInfo.PhysicalBytesPerSectorForAtomicity;
	// - physicalSectorAlignment

	auto [result, optionalNewBuffer] = checkOrCreateAlignedBufferForUnbufferedAccess(readToBuffer, fileSize, physicalSectorAlignment, allowCreateNewBuffer);
	if (result != PrepareAlignedBufferForUnbufferedAccessResult::SUCCESS)
	{
		return {static_cast<ReadFileResult>(result), 0, std::optional<UnbufferedFileAccessBuffer>{}};
	}

	if (optionalNewBuffer)
	{
		newBuffer = std::move(optionalNewBuffer.value());
		sectorAlignedBuffer = {newBuffer.data(), newBuffer.size()};
	} // else: no new buffer so the given buffer is suitable

	uint64_t sectorAlignedFileSize = MathHelper::roundUpToPowerOf2(fileSize, physicalSectorAlignment);

	// the buffer must be large enough
	assert(sectorAlignedBuffer.size() >= sectorAlignedFileSize);

	bool failed = false;
	while (totalBytesRead < fileSize)
	{
		uint64_t bytesToRead = MathHelper::roundDownToPowerOf2(std::min(sectorAlignedFileSize - totalBytesRead, FILE_MAX_BYTES_PER_ACCESS_FUNCTION), physicalSectorAlignment);
		DWORD bytesRead = 0;
		BOOL readResult = ::ReadFile(fileHandle.get(), sectorAlignedBuffer.data() + totalBytesRead, static_cast<DWORD>(bytesToRead), &bytesRead, nullptr);
		if (!readResult)
		{
			failed = true;
			break;
		}

		// assert: (bytesRead % physicalSectorAlignment) == 0 || ((bytesRead % physicalSectorAlignment) == (fileSize - totalBytesRead) && totalBytesRead == roundDownToPowerOf2(fileSize, physicalSectorAlignment))
		// we can not do that assert here because we do not have the values required in this function
		totalBytesRead += bytesRead;
	}

	if (failed)
	{
		return {ReadFileResult::FAILED_READ, 0, std::optional<UnbufferedFileAccessBuffer>{}};
	}

	if (newBuffer)
	{
		return {ReadFileResult::SUCCESS, totalBytesRead, std::optional<UnbufferedFileAccessBuffer>{std::move(newBuffer)}};
	}
	return {ReadFileResult::SUCCESS, totalBytesRead, std::optional<UnbufferedFileAccessBuffer>{}};
}

FileSystemHelper::WriteFileResult FileSystemHelper::writeFileContentUnbuffered(const std::wstring &filePath, const BufferInfo<std::byte> dataBuffer, size_t actualDataSize, bool allowOverwrite, bool preallocate, bool writeThrough, bool allowCreateNewBuffer) noexcept
{
	auto dataSize = actualDataSize;
	DWORD creationDisposition = allowOverwrite ? OPEN_ALWAYS : CREATE_NEW;
	DWORD flagsAndAttributes = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING;
	if (writeThrough)
	{
		flagsAndAttributes |= FILE_FLAG_WRITE_THROUGH;
	}
	// the writes made by this function are unbuffered meaning they bypass windows caching "completely"
	// so the write is extremely fast (compared to normal writes at least) because we are bypassing the OS cache and we are atomically writing to the sectors of the physical device
	// no read-update-write actions required to write some chunk of data
	unique_ptr_file_handle fileHandle{::CreateFileW(filePath.c_str(), GENERIC_WRITE, FILE_OPEN_SHARE_FLAGS, nullptr, creationDisposition, flagsAndAttributes, nullptr), &::CloseHandle};
	if (fileHandle.get() == INVALID_HANDLE_VALUE)
	{
		return WriteFileResult::FAILED_OPEN_FILE;
	}

	if (dataSize > 0)
	{
		// we need to manually get the storage info here because we might need it to calculate the allocationSize before creating the buffer
		// (setting allocation info before creating the buffer is to reduce time between file open and allocation and this is to reduce race conditions)
		// + get physicalSectorAlignment
		auto optionalStorageInfo = getStorageInfo(fileHandle.get());
		if (!optionalStorageInfo)
		{
			return WriteFileResult::FAILED_GET_STORAGE_INFO;
		}
		auto storageInfo = optionalStorageInfo.value();

		// for some reason this value can be "wrong". It might be the logical sector size. So the WriteFile function will still work. But we might loss atomicity for the last sector write
		// these wrong reports can occur with flash drives (maybe because of the filesystem on them?!?)
		uint64_t physicalSectorAlignment = storageInfo.PhysicalBytesPerSectorForAtomicity;
		// - get physicalSectorAlignment

		uint64_t sectorAlignedWriteSize = MathHelper::roundUpToPowerOf2(static_cast<uint64_t>(dataSize), physicalSectorAlignment);
		if (preallocate)
		{
			uint64_t preallocationSize = sectorAlignedWriteSize;

			// here we get the current file's allocation size first ...
			auto optionalSize = getFileSize(fileHandle.get());
			if (optionalSize)
			{
				// ... and select the bigger of our write size or the current file's allocation size
				// to try to not kill the data until we have finished writing
				preallocationSize = std::max(std::get<1>(optionalSize.value()), preallocationSize);

				// now set the allocation info
				// by doing this the OS (or rather the filesystem driver) guarantees us that the file has space for the requested data (only if the function succeeds of course)
				// the function call might take some time depending on the requested size and the speed of the storage device (but it is much much faster than the actual write)
				// doing this has additional advantages: it prevents fragmentation and writing is therefore faster
				// Additional note: There's a special gotcha about setting the file allocation info: If you set the file allocation info to a nonzero value, then the file contents will be forced into nonresident data, even if it would have fit inside the MFT.
				if (!setFileAllocationSize(fileHandle.get(), preallocationSize))
				{
					return WriteFileResult::FAILED_SET_ALLOCATION_INFO;
				}
			} // else: should be stop completely? Or skip prealloc? Or just continue and maybe kill data?!?

			// we currently just skip the prealloc if getting the curent file's allocation size fails
		}

		auto [result, optionalNewBuffer] = checkOrCreateAlignedBufferForUnbufferedAccess(dataBuffer, static_cast<uint64_t>(dataSize), physicalSectorAlignment, allowCreateNewBuffer);
		if (result != PrepareAlignedBufferForUnbufferedAccessResult::SUCCESS)
		{
			return static_cast<WriteFileResult>(result);
		}

		BufferInfo<std::byte> sectorAlignedBuffer = dataBuffer;
		UnbufferedFileAccessBuffer newBuffer;
		if (optionalNewBuffer)
		{
			newBuffer = std::move(optionalNewBuffer.value());
			sectorAlignedBuffer = {newBuffer.data(), newBuffer.size()};
		} // else: no new buffer so the given buffer is suitable

		// the buffer must be large enough
		assert(sectorAlignedBuffer.size() >= sectorAlignedWriteSize);

		if (newBuffer)
		{
			// copy the data to our new buffer
			std::memcpy(newBuffer.data(), dataBuffer.data(), dataSize);
			// fill the reset of our buffer with zeros to ensure we do not leak any (possible sensitive) data to the file
			auto overShotSize = static_cast<size_t>(sectorAlignedWriteSize - dataSize);
			std::memset(newBuffer.data() + dataSize, 0, overShotSize);
		}

		bool failed = false;
		uint64_t totalBytesWritten = 0;

		while (totalBytesWritten < dataSize)
		{
			uint64_t bytesToWrite = MathHelper::roundDownToPowerOf2(std::min(sectorAlignedWriteSize - totalBytesWritten, FILE_MAX_BYTES_PER_ACCESS_FUNCTION), physicalSectorAlignment);
			DWORD bytesWritten = 0;
			// the WriteFile call can not be interrupted. NOT EVEN BY KILLING THE PROCESS. BECAUSE IT CAN NOT BE KILLED DURING THE WriteFile EXECUTION. NO JOKE (power loss is something else of course)
			// it might be possible of course that we have only written some of our data after the call (if we lose power or the file is bigger than 2GB and we need multiple WriteFile calls)
			BOOL readResult = ::WriteFile(fileHandle.get(), sectorAlignedBuffer.data() + totalBytesWritten, static_cast<DWORD>(bytesToWrite), &bytesWritten, nullptr);
			if (!readResult)
			{
				failed = true;
				break;
			}

			// assert: (bytesWritten % physicalSectorAlignment) == 0 || ((bytesWritten % physicalSectorAlignment) == (writeSize - totalBytesRead) && totalBytesRead == roundDownToPowerOf2(writeSize, physicalSectorAlignment))
			// we can not do that assert here because we do not have the values required in this function
			totalBytesWritten += bytesWritten;
		}

		if (failed)
		{
			// TODO: we could try to set the EOF to the size we have (successfully) actually written. Should we do that?
			return WriteFileResult::FAILED_WRITE;
		}
	}

	// truncate the rest of the old data that was there
	// IMPORTANT. This can only be done with SetFileInformationByHandle and FILE_END_OF_FILE_INFO because SetFilePointerEx can only take values multiple of the sector size
	if (!setEndOfFile(fileHandle.get(), static_cast<uint64_t>(dataSize)))
	{
		return WriteFileResult::FAILED_SET_END_OF_FILE;
	}

	return WriteFileResult::SUCCESS;
}

bool FileSystemHelper::setEndOfFile(HANDLE fileHandle, uint64_t fileSize) noexcept
{
	LARGE_INTEGER newFileSize;
	newFileSize.QuadPart = fileSize;
	FILE_END_OF_FILE_INFO endOfFileInfo;
	endOfFileInfo.EndOfFile = newFileSize;
	BOOL result = ::SetFileInformationByHandle(fileHandle, FileEndOfFileInfo, &endOfFileInfo, sizeof(endOfFileInfo));
	if (!result)
	{
		return false;
	}
	return true;
}

bool FileSystemHelper::setFileAllocationSize(HANDLE fileHandle, uint64_t allocationSize) noexcept
{
	LARGE_INTEGER newAllocationSize;
	newAllocationSize.QuadPart = allocationSize;
	FILE_ALLOCATION_INFO allocationInfo;
	allocationInfo.AllocationSize = newAllocationSize;
	BOOL result = ::SetFileInformationByHandle(fileHandle, FileAllocationInfo, &allocationInfo, sizeof(allocationInfo));
	if (!result)
	{
		return false;
	}
	return true;
}

std::optional<std::tuple<uint64_t, uint64_t>> FileSystemHelper::getFileSize(HANDLE fileHandle) noexcept
{
	FILE_STANDARD_INFO standardInfo;
	BOOL result = ::GetFileInformationByHandleEx(fileHandle, FileStandardInfo, &standardInfo, sizeof(standardInfo));
	if (!result)
	{
		return {};
	}

	return {{standardInfo.EndOfFile.QuadPart, standardInfo.AllocationSize.QuadPart}};
}

std::optional<std::tuple<uint64_t, uint64_t>> FileSystemHelper::getFileSize(const std::wstring &filePath) noexcept
{
	unique_ptr_file_handle fileHandle{::CreateFileW(filePath.c_str(), 0, FILE_OPEN_SHARE_FLAGS, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr), &::CloseHandle};
	if (fileHandle.get() == INVALID_HANDLE_VALUE)
	{
		return {};
	}

	return getFileSize(fileHandle.get());
}

std::optional<FILE_STORAGE_INFO> FileSystemHelper::getStorageInfo(HANDLE fileHandle) noexcept
{
	FILE_STORAGE_INFO storageInfo;
	BOOL result = ::GetFileInformationByHandleEx(fileHandle, FileStorageInfo, &storageInfo, sizeof(storageInfo));
	if (!result)
	{
		return {};
	}

	return {storageInfo};
}
