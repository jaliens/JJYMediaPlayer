#pragma once

template<typename T, int SIZE>
class FixedQueue
{
private:
	T arr[SIZE];
	int front, back, count;
public:
	FixedQueue() : front(0), back(0), count(0) {}

	bool nextPtr(T* nextPtr)
	{
		if (this->count >= SIZE)
		{
			return false;
		}

		int returnIndex = this->back;
		this->back = (this->back + 1) % SIZE;
		this->count += 1;
		nextPtr = &(this->arr[returnIndex]);
		return true;

	}

	bool frontPtr(T* frontPtr)
	{
		if (this->count <= 0)
		{
			return false;
		}

		frontPtr = &(this->arr[front]);

		return true;
	}

	bool pop()
	{
		if (this->count <= 0)
		{
			return false;
		}

		this->front = (this->front + 1) % SIZE;
		this->count -= 1;

		return false;
	}
};


