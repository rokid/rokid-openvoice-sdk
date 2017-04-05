#include <stdio.h>
#include "speech.h"

using namespace rokid::speech;

int main(int argc, char** argv) {
	Speech* speech = new_speech();
	speech->config("server_address", "apigw.open.rokid.com:443");
	speech->config("ssl_roots_pem", "etc/roots.pem");
	speech->config("auth_key", "rokid_test_key");
	speech->config("device_type_id", "rokid_test_device_type_id");
	speech->config("device_id", "rokid_test_device_id");
	speech->config("version", "1");
	speech->config("secret", "rokid_test_secret");
	speech->config("codec", "opu");
	if (!speech->prepare()) {
		printf("speech prepare failed\n");
		delete_speech(speech);
		return 1;
	}
	speech->put_text("你好");
	delete_speech(speech);
	printf("ok\n");
	return 0;
}
