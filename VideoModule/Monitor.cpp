#include "pch.h"
#include "Monitor.h"

void Monitor::setConditionFalse()
{
	std::unique_lock<std::mutex> lock(mtx);
	this->ready = false;
}

void Monitor::wait() {
	std::unique_lock<std::mutex> lock(mtx);//뮤텍스 획득
	ready = false;
	cv.wait(lock, [this] { return ready; });//lock 뮤택스 해제하고 대기(대기가 풀리면 뮤택스를 다시 획득 후 지역을 벗어날 때 해제)
}

void Monitor::checkAndWait()
{
	std::unique_lock<std::mutex> lock(mtx);//뮤텍스 획득
	cv.wait(lock, [this] { return ready; });
}



void Monitor::notify() {
	std::lock_guard<std::mutex> lock(mtx);//뮤텍스 획득
	ready = true;//조건 변수 변경
	cv.notify_one();//알림 (주의: 뮤텍스가 해제되어야 wait를 호출한 쪽의 블럭상태가 풀림)
}



void Monitor::requestHoldAndWait()
{
	std::unique_lock<std::mutex> lock(mtx);//뮤텍스 획득
	ready = false;
	ready2 = false;
	cv2.wait(lock, [this] { return ready2; });
}



void Monitor::checkRequestAndWait()
{
	std::unique_lock<std::mutex> lock(mtx);//뮤텍스 획득
	if (ready == false)
	{
		ready2 = true;
		cv2.notify_one();
		cv.wait(lock, [this] { return ready; });
	}
}
