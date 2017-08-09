#include <stdio.h>

void tts_demo();

void speech_demo();

static void ws_demo();

int main(int argc, char** argv) {
	tts_demo();

	speech_demo();

	// ws_demo();

	return 0;
}

#include "Poco/SharedPtr.h"
#include "Poco/Net/PrivateKeyPassphraseHandler.h"
#include "Poco/Net/InvalidCertificateHandler.h"
#include "Poco/Net/ConsoleCertificateHandler.h"
#include "Poco/Net/KeyConsoleHandler.h"
#include "Poco/Net/SSLManager.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/HTTPMessage.h"
#include "Poco/Net/WebSocket.h"
#include "Poco/Net/HTTPSClientSession.h"
#include "Poco/Net/Context.h"
#include "Poco/Exception.h"
#include "auth.pb.h"

using Poco::SharedPtr;
using Poco::Net::PrivateKeyPassphraseHandler;
using Poco::Net::KeyConsoleHandler;
using Poco::Net::InvalidCertificateHandler;
using Poco::Net::ConsoleCertificateHandler;
using Poco::Net::SSLManager;
using Poco::Net::HTTPSClientSession;
using Poco::Net::Context;
using Poco::Net::HTTPRequest;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPMessage;
using Poco::Net::WebSocket;
using Poco::Exception;
using rokid::open::speech::AuthRequest;

static void ws_demo() {
	HTTPRequest request(HTTPRequest::HTTP_GET, "/?encoding=text", HTTPMessage::HTTP_1_1);
	request.set("origin", "https://www.websocket.org");
	HTTPResponse response;

	try {
		Context::Ptr context = new Context(Context::CLIENT_USE, "", "", "etc/roots.pem");
		Poco::Net::initializeSSL();
		SharedPtr<PrivateKeyPassphraseHandler> key_handler = new KeyConsoleHandler(false);
		SharedPtr<InvalidCertificateHandler> cert_handler = new ConsoleCertificateHandler(false);
		SSLManager::instance().initializeClient(key_handler, cert_handler, context);

		HTTPSClientSession cs("echo.websocket.org", 443);
		WebSocket* ws = new WebSocket(cs, request, response);
		AuthRequest req;
		req.set_key("rokid_test_key");
		req.set_device_type_id("rokid_test_device_type_id");
		req.set_device_id("ming");
		req.set_service("tts");
		req.set_version("1");
		req.set_timestamp("00000000");
		req.set_sign("FFFFFFFF");
		std::string reqstr;
		req.SerializeToString(&reqstr);
		printf("send %zu bytes\n", reqstr.length());
		ws->sendFrame(reqstr.data(), reqstr.length(), WebSocket::FRAME_BINARY);
		char buf[256];
		int flags = 0;
		int s = ws->receiveFrame(buf, sizeof(buf), flags);
		printf("recv %d bytes, flags %x\n", s, flags);
		AuthRequest resp;
		std::string respstr(buf, s);
		resp.ParseFromString(respstr);
		printf("recv AuthRequest: key=%s, device type id=%s, device id=%s, service=%s, version=%s\n",
				resp.key().c_str(), resp.device_type_id().c_str(), resp.device_id().c_str(),
				resp.service().c_str(), resp.version().c_str());
		ws->close();
	} catch (Exception &e) {
		printf("Exception: %s\n", e.message().c_str());
	}
}

