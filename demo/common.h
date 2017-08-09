#pragma once

#include <thread>
#include "speech_types.h"

template <typename T>
bool prepare(T* inst) {
	rokid::speech::PrepareOptions opt;
	opt.host = "apigwws.open.rokid.com";
	opt.port = 443;
	opt.branch = "/api";
	opt.key = "rokid_test_key";
	opt.device_type_id = "rokid_test_device_type_id";
	opt.device_id = "rokid_test_device_id";
	opt.secret = "rokid_test_secret";
	return inst->prepare(opt);
}

template <typename T>
void run(T* inst, void (*req_func)(T*), void (*resp_func)(T*)) {
	std::thread th(resp_func, inst);
	req_func(inst);
	th.join();
}
