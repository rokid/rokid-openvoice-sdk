#pragma once

#include <thread>

template <typename T>
bool prepare(T* inst) {
	inst->config("server_address", "apigw.open.rokid.com:443");
	inst->config("ssl_roots_pem", "etc/roots.pem");
	inst->config("key", "rokid_test_key");
	inst->config("device_type_id", "rokid_test_device_type_id");
	inst->config("device_id", "rokid_test_device_id");
	inst->config("version", "1");
	inst->config("secret", "rokid_test_secret");
	return inst->prepare();
}

template <typename T>
void run(T* inst, void (*req_func)(T*), void (*resp_func)(T*)) {
	std::thread th(resp_func, inst);
	req_func(inst);
	th.join();
}
