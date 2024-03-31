#pragma once
#include <mutex>
#include <condition_variable>

class Monitor
{
private:
	std::mutex mtx;
	std::condition_variable cv;
	std::condition_variable cv2;
	bool ready = true;
	bool ready2 = true;
public:

	//���� ���� false�� ����
	void setConditionFalse();
	
	//Ȯ�� ���
	void wait();

	//���Ǻ� ���
	void checkAndWait();

	//��ȣ(��� Ǯ����)
	void notify();

	//��� ��û �� ���
	void requestHoldAndWait();

	//��û Ȯ�� �� ���
	void checkRequestAndWait();
};

