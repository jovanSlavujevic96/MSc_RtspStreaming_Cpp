#pragma once

#include <cstdint>
#include <memory>
#include <queue>
#include <string>

#include "Socket.h"

namespace net
{

void WriteUint32BE(char* p, uint32_t value);
void WriteUint32LE(char* p, uint32_t value);
void WriteUint24BE(char* p, uint32_t value);
void WriteUint24LE(char* p, uint32_t value);
void WriteUint16BE(char* p, uint16_t value);
void WriteUint16LE(char* p, uint16_t value);
	
class BufferWriter final
{
public:
	BufferWriter(int capacity = kMaxQueueLength);
	~BufferWriter() = default;

	bool Append(std::shared_ptr<char> data, uint32_t size, uint32_t index=0);
	bool Append(const char* data, uint32_t size, uint32_t index=0);
	int Send(SOCKET sockfd, int timeout=0);

	bool IsEmpty() const;
	bool IsFull() const;
	size_t Size() const;
	
private:
	struct Packet
	{
		std::shared_ptr<char> data = nullptr;
		uint32_t size = 0;
		uint32_t writeIndex = 0;
	};

	std::queue<std::unique_ptr<Packet>> buffer_;  		
	int max_queue_length_ = 0;
	 
	static constexpr size_t kMaxQueueLength = 10000;
};

}
