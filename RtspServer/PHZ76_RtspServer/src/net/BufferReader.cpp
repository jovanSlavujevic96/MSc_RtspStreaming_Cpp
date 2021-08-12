#include "BufferReader.h"
#include "Socket.h"
#include <cstring>
 
namespace net
{

uint32_t net::ReadUint32BE(char* data)
{
	uint8_t* p = (uint8_t*)data;
	uint32_t value = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
	return value;
}

uint32_t net::ReadUint32LE(char* data)
{
	uint8_t* p = (uint8_t*)data;
	uint32_t value = (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];
	return value;
}

uint32_t net::ReadUint24BE(char* data)
{
	uint8_t* p = (uint8_t*)data;
	uint32_t value = (p[0] << 16) | (p[1] << 8) | p[2];
	return value;
}

uint32_t net::ReadUint24LE(char* data)
{
	uint8_t* p = (uint8_t*)data;
	uint32_t value = (p[2] << 16) | (p[1] << 8) | p[0];
	return value;
}

uint16_t net::ReadUint16BE(char* data)
{
	uint8_t* p = (uint8_t*)data;
	uint16_t value = (p[0] << 8) | p[1];
	return value;
}

uint16_t net::ReadUint16LE(char* data)
{
	uint8_t* p = (uint8_t*)data;
	uint16_t value = (p[1] << 8) | p[0];
	return value;
}

#if __cplusplus < 201703L
const char* BufferReader::kCRLF = "\r\n";
const uint32_t BufferReader::MAX_BYTES_PER_READ = 4096;
const uint32_t BufferReader::MAX_BUFFER_SIZE = 1024 * 100000;
#endif

BufferReader::BufferReader(uint32_t initial_size)
{
	buffer_.resize(initial_size);
}

uint32_t BufferReader::ReadableBytes() const
{
	return (uint32_t)(writer_index_ - reader_index_);
}

uint32_t BufferReader::WritableBytes() const
{
	return (uint32_t)(buffer_.size() - writer_index_);
}

char* BufferReader::Peek()
{
	return Begin() + reader_index_;
}

const char* BufferReader::Peek() const
{
	return Begin() + reader_index_;
}

const char* BufferReader::FindFirstCrlf() const 
{
	const char* crlf = std::search(Peek(), BeginWrite(), kCRLF, kCRLF + 2);
	return crlf == BeginWrite() ? nullptr : crlf;
}

const char* BufferReader::FindLastCrlf() const 
{
	const char* crlf = std::find_end(Peek(), BeginWrite(), kCRLF, kCRLF + 2);
	return crlf == BeginWrite() ? nullptr : crlf;
}

const char* BufferReader::FindLastCrlfCrlf() const 
{
#if __cplusplus >= 201703L
	static constexpr char* crlfCrlf = "\r\n\r\n";
#else
	static const char* crlfCrlf = "\r\n\r\n";
#endif
	const char* crlf = std::find_end(Peek(), BeginWrite(), crlfCrlf, crlfCrlf + 4);
	return crlf == BeginWrite() ? nullptr : crlf;
}

void BufferReader::RetrieveAll() 
{
	writer_index_ = 0;
	reader_index_ = 0;
}

void BufferReader::Retrieve(size_t len) 
{
	if (len <= ReadableBytes()) 
	{
		reader_index_ += len;
		if (reader_index_ == writer_index_) 
		{
			reader_index_ = 0;
			writer_index_ = 0;
		}
	}
	else 
	{
		RetrieveAll();
	}
}

void BufferReader::RetrieveUntil(const char* end)
{
	Retrieve(end - Peek());
}

int BufferReader::Read(SOCKET sockfd)
{
	uint32_t size = WritableBytes();
	if (size < MAX_BYTES_PER_READ) 
	{
		uint32_t bufferReaderSize = (uint32_t)buffer_.size();
		if (bufferReaderSize > MAX_BUFFER_SIZE) 
		{
			return 0;
		}
		buffer_.resize(bufferReaderSize + MAX_BYTES_PER_READ);
	}

	int bytes_read = ::recv(sockfd, beginWrite(), MAX_BYTES_PER_READ, 0);
	if (bytes_read > 0) 
	{
		writer_index_ += bytes_read;
	}
	return bytes_read;
}


uint32_t BufferReader::ReadAll(std::string& data)
{
	uint32_t size = ReadableBytes();
	if (size > 0) 
	{
		data.assign(Peek(), size);
		writer_index_ = 0;
		reader_index_ = 0;
	}
	return size;
}

uint32_t BufferReader::ReadUntilCrlf(std::string& data)
{
	const char* crlf = FindLastCrlf();
	if (crlf == nullptr) 
	{
		return 0;
	}
	uint32_t size = (uint32_t)(crlf - Peek() + 2);
	data.assign(Peek(), size);
	Retrieve(size);
	return size;
}

uint32_t BufferReader::Size() const
{
	return (uint32_t)buffer_.size();
}

// private

char* BufferReader::Begin()
{
	return &*buffer_.begin();
}

const char* BufferReader::Begin() const
{
	return &*buffer_.begin();
}

char* BufferReader::beginWrite()
{
	return Begin() + writer_index_;
}

const char* BufferReader::BeginWrite() const
{
	return Begin() + writer_index_;
}

}
