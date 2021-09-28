#pragma once

#include <fstream>
#include <cstdint>

class H264File
{
public:
	H264File(int32_t buf_size = 500000);
	~H264File();

	bool Open(const char* path);
	void Close();
	bool IsOpened() const;
	int ReadFrame(char* in_buf, int32_t in_buf_size, bool* end);

private:
	FILE* m_file = NULL;
	char* m_buf = NULL;
	int32_t m_buf_size = 0;
	int32_t m_bytes_used = 0;
	int32_t m_count = 0;
};
