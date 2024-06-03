#pragma once
#include <cstdint>

//원형큐 버퍼
class FixedMemQueue
{
private:
	uint8_t* data = nullptr;
	uint64_t front = 0;//시작 바이트의 위치
	uint64_t back = 0;//마지막 바이트의 위치
	uint64_t capacity = 0;//전체 바이트 용량
	uint64_t currentSize = 0;//현재 차있는 바이트 용량

	uint64_t* header = nullptr;
	uint64_t headerFront = 0;//시작 아이템 위치
	uint64_t headerBack = 0;//마지막 아이템 위치
	uint64_t capacityCount = 0;//전체 개수
	uint64_t currentCount = 0;//현재 차있는 개수

public:
	FixedMemQueue(uint64_t capacity)
	{
		data = new uint8_t[capacity];
		memset(data,0, capacity);
	}
	FixedMemQueue(uint64_t capacityCount, uint64_t unitSize)
	{
		uint64_t capacity = capacityCount * unitSize;
		data = new uint8_t[capacity];
		header = new uint64_t[capacityCount];
		memset(data, 0, capacity);
		memset(header, 0, capacityCount);
	}

	bool canPush(uint64_t pushSize)
	{
		if (pushSize > capacity - currentSize)
		{
			return false;
		}
		
		return true;
	}

	bool isLittleEndian()
	{
		uint32_t number = 1;
		uint32_t shifted = number >> 24;
		if (shifted == 0)
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	bool canPushPts(uint64_t pts)
	{
		if (currentCount == capacityCount)
		{
			return false;
		}

		return true;
	}

	bool pushPts(uint64_t pts)
	{
		//가득차있어서 못 넣음
		if (currentCount == capacityCount)
		{
			return false;
		}

		headerBack = (headerBack + 1) % capacityCount;
		header[headerBack] = pts;
		return true;
	}

	bool popPts()
	{
		if (currentCount == 0)
		{
			return false;
		}

		header[headerFront] = 0;
		if (headerFront < headerBack)
		{
			headerFront = (headerFront + 1) % capacityCount;
		}
		return true;
	}

	bool push(uint8_t * src, uint64_t pushSize)
	{
		if (pushSize > capacity - currentSize)
		{
			return false;
		}

		uint64_t lastIndex = capacity - 1;

		if (front <= back)
		{
			if (back + pushSize <= lastIndex)
			{
				memcpy(data + back, src, pushSize);
				back = back + pushSize;
			}
			else
			{
				memcpy(data + back, src, lastIndex - back);
				memcpy(data, src + lastIndex - back, pushSize - (lastIndex - back));
				back = pushSize - (lastIndex - back) - 1;
			}
		}
		else if (front > back)
		{
			memcpy(data + back, src, pushSize);
			back = back + pushSize;
		}

		currentSize += pushSize;

		return true;
	}

	bool pop(uint64_t popSize)
	{
		if (popSize > currentSize)
		{
			return false;
		}

		uint64_t lastIndex = capacity - 1;

		if (front + popSize - 1 <= lastIndex)
		{
			memset(data + front,0, popSize);
			front = (front + popSize) % capacity;
		}
		else
		{
			memset(data + front, 0, lastIndex - front + 1);
			memset(data, 0, popSize - (lastIndex - front + 1));
			front = (popSize - (lastIndex - front + 1)) % capacity;
		}

		currentSize -= popSize;

		return true;
	}

	uint8_t* peekFront()
	{
		if (currentSize == 0)
		{
			return nullptr;
		}

		return data + front;
	}

};

