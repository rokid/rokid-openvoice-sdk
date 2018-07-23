#include "trace-uploader.h"
#include <unistd.h>

using namespace std;

int main(int argc, char** argv) {
	string dev_id = "mingx86trace";
	string dev_tp_id = "B16B2DFB5A004DCBAFD0C0291C211CE1";
	TraceUploader uploader(dev_id, dev_tp_id);
	shared_ptr<TraceEvent> ev = make_shared<TraceEvent>();
	ev->type = TRACE_EVENT_TYPE_SYS;
	ev->id = "system.speech.timeout";
	ev->name = "speech服务超时无响应";
	ev->add_key_value("reqid", "1");
	uploader.put(ev);
	sleep(1);
	uploader.put(ev);
	sleep(2);
	return 0;
}
