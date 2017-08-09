package com.rokid.speech;

import java.io.FileInputStream;
import java.io.IOException;
import java.util.List;
import java.lang.reflect.Type;
import android.util.Log;
import org.json.JSONObject;

class GenericConfig {
	private static final String TAG = "speech.GenericConfig";

	protected PrepareOptions parseConfigFile(String configFile) {
		FileInputStream is = null;
		byte[] content;
		String jsonstr;

		try {
			is = new FileInputStream(configFile);
			int size = is.available();
			content = new byte[size];
			is.read(content);
			jsonstr = new String(content);
			Log.d(TAG, "config file content = " + jsonstr);
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

		special_config(json_obj);

		return opt;
	}

	// override it to do special config
	protected void special_config(JSONObject json_obj) {
	}
}
