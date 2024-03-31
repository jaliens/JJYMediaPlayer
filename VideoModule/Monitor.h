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

	//조건 변수 false로 설정
	void setConditionFalse();
	
	//확정 대기
	void wait();

	//조건부 대기
	void checkAndWait();

	//신호(대기 풀어줌)
	void notify();

	//대기 요청 후 대기
	void requestHoldAndWait();

	//요청 확인 후 대기
	void checkRequestAndWait();
};

