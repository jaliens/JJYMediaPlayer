#include "pch.h"
#include "Monitor.h"

void Monitor::setConditionFalse()
{
	std::unique_lock<std::mutex> lock(mtx);
	this->ready = false;
}

void Monitor::wait() {
	std::unique_lock<std::mutex> lock(mtx);//���ؽ� ȹ��
	ready = false;
	cv.wait(lock, [this] { return ready; });//lock ���ý� �����ϰ� ���(��Ⱑ Ǯ���� ���ý��� �ٽ� ȹ�� �� ������ ��� �� ����)
}

void Monitor::checkAndWait()
{
	std::unique_lock<std::mutex> lock(mtx);//���ؽ� ȹ��
	cv.wait(lock, [this] { return ready; });
}



void Monitor::notify() {
	std::lock_guard<std::mutex> lock(mtx);//���ؽ� ȹ��
	ready = true;//���� ���� ����
	cv.notify_one();//�˸� (����: ���ؽ��� �����Ǿ�� wait�� ȣ���� ���� �����°� Ǯ��)
}


//��⸦ ��û�ϰ� ����� ����� �� ���� ��ٸ�
void Monitor::requestHoldAndWait()
{
	std::unique_lock<std::mutex> lock(mtx);//���ؽ� ȹ��
	ready = false;
	ready2 = false;
	cv2.wait(lock, [this] { return ready2; });
}


//ȣ�� ������ ��� ��û�� ������ ��û���� ��⸦ Ǯ���ְ� ȣ���ڴ� ���
void Monitor::checkRequestAndWait()
{
	std::unique_lock<std::mutex> lock(mtx);//���ؽ� ȹ��
	if (ready == false)
	{
		ready2 = true;
		cv2.notify_one();
		cv.wait(lock, [this] { return ready; });
	}
}
