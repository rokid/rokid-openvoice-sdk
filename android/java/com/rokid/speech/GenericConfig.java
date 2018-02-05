package com.rokid.speech;

import java.io.InputStream;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.List;
import java.lang.reflect.Type;
import android.util.Log;
import org.json.JSONObject;

public class GenericConfig {
	private static final String TAG = "speech.GenericConfig";

	public PrepareOptions parseConfigFile(String configFile) {
		FileInputStream is = null;
		try {
			is = new FileInputStream(configFile);
			return parseConfig(is);
		} catch (Exception e) {
			e.printStackTrace();
			return null;
		} finally {
			if (is != null) {
				try {
					is.close();
				} catch (IOException e) {
				}
			}
		}
	}

	public PrepareOptions parseConfig(InputStream is) {
		byte[] content;
		try {
			int size = is.available();
			content = new byte[size];
			is.read(content);
		} catch (Exception e) {
			e.printStackTrace();
			return null;
		}
		return parseConfig(content);
	}

	public PrepareOptions parseConfig(byte[] content) {
		return parseConfig(content, 0, content.length);
	}

	public PrepareOptions parseConfig(byte[] content, int offset, int length) {
		String jsonstr = new String(content, offset, length);
		return parseConfig(jsonstr);
	}

	public PrepareOptions parseConfig(String jsonstr) {
		JSONObject json_obj;
		PrepareOptions opt;
		try {
			json_obj = new JSONObject(jsonstr);
		} catch (Exception e) {
			e.printStackTrace();
			return null;
		}
		opt = new PrepareOptions();
		opt.host = json_obj.optString("host", "localhost");
		opt.port = json_obj.optInt("port", 80);
		opt.branch = json_obj.optString("branch", "/");
		opt.key = json_obj.optString("key", null);
		opt.device_type_id = json_obj.optString("device_type_id", null);
		opt.secret = json_obj.optString("secret", null);
		opt.device_id = json_obj.optString("device_id", null);

		int v;
		v = json_obj.optInt("reconn", 0);
		if (v > 0)
			opt.reconn_interval = v;
		v = json_obj.optInt("keepalive", 0);
		if (v > 0)
			opt.ping_interval = v;
		v = json_obj.optInt("noresp_timeout", 0);
		if (v > 0)
			opt.no_resp_timeout = v;

		special_config(json_obj);

		return opt;
	}

	// override it to do special config
	protected void special_config(JSONObject json_obj) {
	}
}
