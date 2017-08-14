package com.rokid.speech;

import java.io.FileInputStream;
import java.io.IOException;
import java.util.List;
import java.lang.reflect.Type;
import android.util.Log;
import com.alibaba.fastjson.JSON;
import com.alibaba.fastjson.TypeReference;
import com.alibaba.fastjson.parser.Feature;

abstract class GenericConfig<T extends GenericConfigParams> {
	private static final String TAG = "speech.GenericConfig";

	protected void config(String configFile, Class<T> cls) {
		FileInputStream is = null;
		byte[] content;

		try {
			is = new FileInputStream(configFile);
			int size = is.available();
			Log.d(TAG, "config file size = " + size);
			content = new byte[size];
			is.read(content);
			Log.d(TAG, "config file content = " + new String(content));
		} catch (Exception e) {
			e.printStackTrace();
			return;
		} finally {
			if (is != null) {
				try {
					is.close();
				} catch (IOException e) {
				}
			}
		}
		T config = JSON.parseObject(new String(content), cls);
		Log.d(TAG, "config file parse result = " + config);
		config("host", config.host);
		config("port", config.port);
		config("branch", config.branch);
		config("key", config.key);
		config("device_type_id", config.device_type_id);
		config("device_id", config.device_id);
		config("secret", config.secret);
		config("api_version", config.api_version);

		special_config(config);
	}

	public abstract void config(String key, String value);

	// override it to do special config
	protected void special_config(T conf) {
	}
}

class GenericConfigParams {
	public String host;

	public String port;

	public String branch;

	public String key;

	public String device_type_id;

	public String device_id;

	public String secret;

	public String api_version;

	public String toString() {
		return "host=" + host
			+ ":" + port
			+ "" + branch
			+ ", key=" + key
			+ ", device_type_id=" + device_type_id
			+ ", device_id=" + device_id
			+ ", secret=" + secret
			+ ", api_version=" + api_version;
	}
}
