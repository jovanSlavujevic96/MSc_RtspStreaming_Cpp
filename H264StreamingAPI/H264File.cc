#include "H264File.h"

H264File::H264File(int32_t buf_size) :
	m_buf_size(buf_size)
{
	m_buf = new char[m_buf_size];
}

H264File::~H264File()
{
	delete [] m_buf;
}

bool H264File::Open(const char* path)
{
#if defined(__linux) || defined(__linux__) 
	m_file = fopen(path, "rb");
#elif defined(WIN32) || defined(_WIN32) 
	fopen_s(&m_file, path, "rb");
#endif
	if (m_file == NULL) 
	{
		return false;
	}
	return true;
}

void H264File::Close()
{
	if (m_file) 
	{
		fclose(m_file);
		m_file = NULL;
		m_count = 0;
		m_bytes_used = 0;
	}
}

bool H264File::IsOpened() const
{
	return (m_file != NULL);
}

int H264File::ReadFrame(char* in_buf, int32_t in_buf_size, bool* end)
{
	if (m_file == NULL) 
	{
		return -1;
	}

	int bytes_read = (int)fread(m_buf, 1, m_buf_size, m_file);
	if (bytes_read == 0) 
	{
		fseek(m_file, 0, SEEK_SET);
		m_count = 0;
		m_bytes_used = 0;
		bytes_read = (int)fread(m_buf, 1, m_buf_size, m_file);
		if (bytes_read == 0) 
		{
			this->Close();
			return -1;
		}
	}

	bool is_find_start = false, is_find_end = false;
	int i = 0, start_code = 3;
	*end = false;

	for (i = 0; i < bytes_read - 5; i++) 
	{
		if (m_buf[i] == 0 && m_buf[i + 1] == 0 && m_buf[i + 2] == 1) 
		{
			start_code = 3;
		}
		else if (m_buf[i] == 0 && m_buf[i + 1] == 0 && m_buf[i + 2] == 0 && m_buf[i + 3] == 1) 
		{
			start_code = 4;
		}
		else 
		{
			continue;
		}

		if (((m_buf[i + start_code] & 0x1F) == 0x5 || (m_buf[i + start_code] & 0x1F) == 0x1)
			&& ((m_buf[i + start_code + 1] & 0x80) == 0x80)) 
		{
			is_find_start = true;
			i += 4;
			break;
		}
	}

	for (; i < bytes_read - 5; i++) 
	{
		if (m_buf[i] == 0 && m_buf[i + 1] == 0 && m_buf[i + 2] == 1)
		{
			start_code = 3;
		}
		else if (m_buf[i] == 0 && m_buf[i + 1] == 0 && m_buf[i + 2] == 0 && m_buf[i + 3] == 1) 
		{
			start_code = 4;
		}
		else 
		{
			continue;
		}

		if (((m_buf[i + start_code] & 0x1F) == 0x7) || ((m_buf[i + start_code] & 0x1F) == 0x8)
			|| ((m_buf[i + start_code] & 0x1F) == 0x6) || (((m_buf[i + start_code] & 0x1F) == 0x5
				|| (m_buf[i + start_code] & 0x1F) == 0x1) && ((m_buf[i + start_code + 1] & 0x80) == 0x80))) 
		{
			is_find_end = true;
			break;
		}
	}

	bool flag = false;
	if (is_find_start && !is_find_end && m_count > 0) 
	{
		flag = is_find_end = true;
		i = bytes_read;
		*end = true;
	}

	if (!is_find_start || !is_find_end) 
	{
		this->Close();
		return -1;
	}

	int size = (i <= in_buf_size ? i : in_buf_size);
	memcpy(in_buf, m_buf, size);

	if (!flag) 
	{
		m_count += 1;
		m_bytes_used += i;
	}
	else 
	{
		m_count = 0;
		m_bytes_used = 0;
	}

	fseek(m_file, m_bytes_used, SEEK_SET);
	return size;
}
