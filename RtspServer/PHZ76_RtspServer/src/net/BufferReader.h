#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <algorithm>  
#include <memory>  
#include "Socket.h"

namespace net
{

uint32_t ReadUint32BE(char* data);
uint32_t ReadUint32LE(char* data);
uint32_t ReadUint24BE(char* data);
uint32_t ReadUint24LE(char* data);
uint16_t ReadUint16BE(char* data);
uint16_t ReadUint16LE(char* data);
    
class BufferReader final
{
public:	
	BufferReader(uint32_t initial_size = 2048);
	~BufferReader() = default;

	uint32_t ReadableBytes() const;
	uint32_t WritableBytes() const;

	char* Peek();
	const char* Peek() const;
	const char* FindFirstCrlf() const;
	const char* FindLastCrlf() const;
	const char* FindLastCrlfCrlf() const;
	void RetrieveAll();
	void Retrieve(size_t len);
	void RetrieveUntil(const char* end);

	int Read(SOCKET sockfd);
	uint32_t ReadAll(std::string& data);
	uint32_t ReadUntilCrlf(std::string& data);
	uint32_t Size() const;

private:
	char* Begin();
	const char* Begin() const;
	char* beginWrite();
	const char* BeginWrite() const;

	std::vector<char> buffer_;
	size_t reader_index_ = 0;
	size_t writer_index_ = 0;

#if __cplusplus >= 201703L
	static inline constexpr char* kCRLF = "\r\n";
	static inline constexpr uint32_t MAX_BYTES_PER_READ = 4096;
	static inline constexpr uint32_t MAX_BUFFER_SIZE = 1024 * 100000;
#else
	static const char* kCRLF;
	static const uint32_t MAX_BYTES_PER_READ;
	static const uint32_t MAX_BUFFER_SIZE;
#endif
};

}
