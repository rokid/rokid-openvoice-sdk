#include <vector>
#include <thread>
#include "alt_chrono.h"
#include <mutex>
#include <list>
#include "speech.h"

typedef struct {
	int32_t id;
	std::chrono::milliseconds duration;
	int32_t error;
} FailedSpeech;

typedef struct {
	int32_t id;
	SteadyClock::time_point start_tp;
} SpeechReqInfo;

class SpeechStressTest {
public:
	static void test(const rokid::speech::PrepareOptions& opts, const char* pcm_file, uint32_t repeat);

private:
	void run(std::shared_ptr<rokid::speech::Speech>& sp, uint8_t* pcm, uint32_t size, uint32_t repeat);

	void init();

	void final();

	void run_speech_poll();

	void add_speech_req(int32_t id);

	SpeechReqInfo* get_top_speech_req();

	void pop_speech_req();

	void handle_speech_result(SpeechReqInfo* reqinfo, const rokid::speech::SpeechResult& result);

	void wait_pending_reqs();

	void print_test_result();

private:
	std::shared_ptr<rokid::speech::Speech> speech;
	int32_t cur_speech_id;
	std::thread* poll_thread;
	uint32_t speech_req_count;
	uint32_t speech_req_success;
	std::vector<FailedSpeech> failed_speech;
	std::list<SpeechReqInfo> pending_speech_reqs;
	std::mutex list_mutex;
};
