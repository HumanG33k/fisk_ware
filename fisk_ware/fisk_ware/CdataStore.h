#ifndef DATASTORE_H
#define DATASTORE_H

//#define NDEBUG
#include <assert.h>
#include <stdlib.h>

class CDataStore
{
public:
	unsigned char* m_buffer;
	unsigned int m_base;        // base offset
	unsigned int m_alloc;        // amount of space allocated, -1 = no ownership of data
	unsigned int m_size;        // total written data (write position)
	int bla;
	unsigned int m_read;        // read position. -1 when not finalized.

protected:

	// Buffer Control
	virtual void InternalInitialize(unsigned char*& buffer, unsigned int&, unsigned int&);
	virtual void InternalDestroy(unsigned char*& buffer, unsigned int&, unsigned int&);
	virtual int InternalFetchRead(unsigned int, unsigned int size, unsigned char*& data, unsigned int&, unsigned int&);
	virtual int InternalFetchWrite(unsigned int, unsigned int, unsigned char*&, unsigned int&, unsigned int&);

	// Cleanup / Destroy
	void Initialize();
	void Destroy();

	// Misc.
	int FetchWrite(unsigned int, unsigned int);

public:

	// Create an empty buffer for writing
	CDataStore() : m_buffer(0), m_base(0), m_alloc(0),
		m_size(0), m_read((unsigned int)-1)
	{
		Initialize();
	}

	// Read an already created buffer. Read-Only (no writing)
	CDataStore(void* data, int length) : m_buffer((unsigned char*)data), m_base(0),
		m_alloc((unsigned int)-1), m_size(length), m_read(0)
	{}

	virtual ~CDataStore() { Destroy(); }

	virtual void Reset();
	virtual int IsRead();
	virtual void Finalize();

	class CDataStore& PutData(const void *, unsigned int);
	class CDataStore& PutString(const char*);
	class CDataStore& GetString(char*, unsigned int);
	class CDataStore& GetData(void*, unsigned int);
	class CDataStore& GetDataInSitu(void*&, unsigned int);

	template <typename T> CDataStore& Set(unsigned int pos, T val)
	{
		assert(!IsFinal());
		assert(pos + sizeof(T) <= m_size);
		if ((pos < m_base) || (pos + sizeof(T) > m_alloc + m_base)) {
			assert(InternalFetchWrite(pos, sizeof(val), m_buffer, m_base, m_alloc));
		}
		*(T*)(m_buffer - m_base + pos) = val;
		return *this;
	}

	template <typename T> CDataStore& Put(T val)
	{
		assert(!IsFinal());
		if ((m_size < m_base) || (m_size + sizeof(T) > m_alloc + m_base))
		{
			// make sure we can write sizeof(T) data
			assert(AssertFetchWrite(m_size, sizeof(T)));
		}
		T* pos = (T*)(m_buffer - m_base + m_size);
		*pos = val;
		m_size += sizeof(T);
		return *this;
	}

	template <typename T> CDataStore& Get(T& val)
	{
		assert(IsFinal());

		unsigned int bytes = m_read + sizeof(T);
		if (bytes > m_size)
		{
			m_read = m_size + 1;
			return *this;
		}

		if ((m_read < m_base) || (bytes > m_alloc + m_base))
		{
			if (!AssertFetchRead(m_read, sizeof(T)))
			{
				m_read = m_alloc + 1;
				return *this;
			}
		}
		val = *(T*)(m_buffer - m_base + m_read);
		m_read += sizeof(T);
		return *this;
	}

	template <typename T> CDataStore& PutArray(const T* pVal, unsigned int count)
	{
		assert(!IsFinal());
		count *= sizeof(T);
		unsigned int pos = count;

		if (pVal != 0)
		{
			if ((m_size < m_base) || (m_size + count > m_alloc))
			{
				InternalFetchWrite(m_size, count, m_buffer, m_base, m_alloc);
			}
			while (count > 0)
			{
				if (pos >= m_alloc)
					pos = m_alloc;
				else
					pos = count;

				if (pos < sizeof(T))
					pos = sizeof(T);

				if ((m_size >= m_base) && (m_size + pos < m_alloc + m_base))
				{
					InternalFetchWrite(m_size, pos, m_buffer, m_base, m_alloc);
				}

				if ((T*)(m_size - m_base + m_buffer) != pVal)
					memcpy(m_buffer + m_size, pVal, count);

				pVal += pos;
				m_size += pos;
				count -= pos;
			}
		}

		return *this;
	}

	template <typename T> CDataStore& GetArray(T* pVal, unsigned int count)
	{
		assert(IsFinal());

		// total amount to be copied
		unsigned int total = count;
		if ((pVal != 0) && (m_read <= m_size) && (count != 0))
		{
			do {
				unsigned int len = m_size - m_read;

				// shave off the length to avoid overflow
				if (len > count) len = count;
				if (len > m_alloc) len = m_alloc;
				if (len < sizeof(T)) len = sizeof(T);

				count = m_read + len;
				if (count > m_size)
				{
					m_read = m_size + sizeof(T);
					return *this;
				}

				// check to make sure we can read
				if ((m_read < (unsigned int)m_base) || (count > m_base + m_alloc))
				{
					if (!AssertFetchRead(m_read, len))
					{
						m_read = m_size + sizeof(T);
						return *this;
					}
				}
				if (pVal != (T*)(m_buffer - m_base + m_read))
				{
					memcpy(pVal, m_buffer - m_base + m_read, len * sizeof(T));
				}
				m_read = m_read + len;
				pVal += len;
				total -= len;
			} while (total > 0);
		}
		return *this;
	}

	template <typename T> CDataStore& operator << (T val)
	{
		return Put(val);
	}

	template <typename T> CDataStore& operator >> (T& val)
	{
		return Get(val);
	}

	void* Buffer() { return m_buffer; }
	int Size() { return m_size; }
	bool IsFinal() { return m_read != -1; }

	// Assertion Methods
	bool AssertFetchWrite(unsigned int pos, unsigned int bytes);
	bool AssertFetchRead(unsigned int pos, unsigned int bytes);

	// Buffer Related
	virtual void GetBufferParams(const void**, unsigned int*, unsigned int*);
	virtual void DetachBuffer(void**, unsigned *, unsigned int*);
};

#endif // DATASTORE_H