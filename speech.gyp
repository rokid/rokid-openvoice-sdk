{
	'variables': {
		'deps_dir%': '/usr/local',
		'src_dir': 'src',
		'proto_gen_dir': 'gen',
		'test_dir': 'test',
	},
	'target_defaults': {
		'configurations': {
			'Debug': {
				'defines': [ 'DEBUG', '_DEBUG' ],
				'cflags': [ '-g', '-O0', '-fno-inline-functions' ],
			},
			'Release': {
				'cflags': [ '-O2' ],
			},
		},
		'default_configuration': 'Debug',
		'cflags': [ '-std=c++11' ],
		'target_conditions': [
			[ '_type=="shared_library"', { 'cflags': [ '-fPIC' ] } ],
		],
	},
	'targets': [
	{
		'target_name': 'common',
		'type': 'shared_library',
		'sources': [
			'<(proto_gen_dir)/speech.pb.cc',
			'<(proto_gen_dir)/speech.grpc.pb.cc',
			'<(src_dir)/common/speech_config.cc',
			'<(src_dir)/common/speech_config.h',
			'<(src_dir)/common/log.h',
			'<(src_dir)/common/log.cc',
			'<(src_dir)/common/speech_connection.h',
			'<(src_dir)/common/speech_connection.cc',
			'<(src_dir)/common/md5.cc',
			'<(src_dir)/common/md5.h',
		],
		'include_dirs': [
			'<(proto_gen_dir)',
			'<(deps_dir)/include',
		],
		'direct_dependent_settings': {
			'include_dirs': [ '<(proto_gen_dir)', '<(deps_dir)/include', '<(src_dir)/common' ],
		},
		'link_settings': {
			'libraries': [ '-L<(deps_dir)/lib', '-lgrpc++', '-lgrpc', '-lprotobuf', '-Wl,-rpath,<(deps_dir)/lib' ],
		},
	}, # target 'common'
	{
		'target_name': 'tts',
		'type': 'shared_library',
		'dependencies': [
			'common',
		],
		'include_dirs': [
			'include',
		],
		'sources': [
			'<(src_dir)/common/pending_queue.h',
			'<(src_dir)/common/pipeline.h',
			'<(src_dir)/common/pipeline_handler.h',
			'<(src_dir)/common/cancel_pipeline_handler.h',
			'<(src_dir)/tts/types.h',
			'<(src_dir)/tts/tts_impl.cc',
			'<(src_dir)/tts/tts_impl.h',
			'<(src_dir)/tts/tts_req_provider.cc',
			'<(src_dir)/tts/tts_req_provider.h',
			'<(src_dir)/tts/tts_req_handler.cc',
			'<(src_dir)/tts/tts_req_handler.h',
			'<(src_dir)/tts/tts_resp_handler.cc',
			'<(src_dir)/tts/tts_resp_handler.h',
			'<(src_dir)/tts/tts_cancel_handler.cc',
			'<(src_dir)/tts/tts_cancel_handler.h',
		],
	}, # target 'tts'
	{
		'target_name': 'asr',
		'type': 'shared_library',
		'dependencies': [
			'common',
		],
		'sources': [
			'<(src_dir)/common/pending_queue.h',
			'<(src_dir)/common/pipeline.h',
			'<(src_dir)/common/pipeline_handler.h',
			'<(src_dir)/common/cancel_pipeline_handler.h',
			'<(src_dir)/asr/types.h',
			'<(src_dir)/asr/asr_impl.cc',
			'<(src_dir)/asr/asr_impl.h',
			'<(src_dir)/asr/asr_req_provider.cc',
			'<(src_dir)/asr/asr_req_provider.h',
			'<(src_dir)/asr/asr_req_handler.cc',
			'<(src_dir)/asr/asr_req_handler.h',
			'<(src_dir)/asr/asr_resp_handler.cc',
			'<(src_dir)/asr/asr_resp_handler.h',
			'<(src_dir)/asr/asr_cancel_handler.cc',
			'<(src_dir)/asr/asr_cancel_handler.h',
		],
		'include_dirs': [
			'include',
		],
	}, # target 'asr'
	{
		'target_name': 'speech',
		'type': 'shared_library',
		'dependencies': [
			'common',
		],
		'include_dirs': [
			'include',
		],
		'sources': [
			'<(src_dir)/common/pending_queue.h',
			'<(src_dir)/common/pipeline.h',
			'<(src_dir)/common/pipeline_handler.h',
			'<(src_dir)/common/cancel_pipeline_handler.h',
			'<(src_dir)/speech/speech_impl.h',
			'<(src_dir)/speech/speech_impl.cc',
			'<(src_dir)/speech/speech_req_provider.h',
			'<(src_dir)/speech/speech_req_provider.cc',
			'<(src_dir)/speech/speech_req_handler.h',
			'<(src_dir)/speech/speech_req_handler.cc',
			'<(src_dir)/speech/speech_resp_handler.h',
			'<(src_dir)/speech/speech_resp_handler.cc',
			'<(src_dir)/speech/speech_cancel_handler.h',
			'<(src_dir)/speech/speech_cancel_handler.cc',
		],
		'direct_dependent_settings': {
			'include_dirs': [ 'include' ],
			'link_settings': {
				'libraries': [ '-L<(deps_dir)/lib', '-lgrpc++', '-lprotobuf', '-Wl,-rpath,<(deps_dir)/lib' ],
			},
		},
		'include_dirs': [
			'include',
		],
	}, # target 'speech'
	{
		'target_name': 'demo',
		'type': 'executable',
		'dependencies': [
			'speech',
			'asr',
			'tts',
			'common',
		],
		'include_dirs': [
			'include',
			'src/common',
		],
		'sources': [
			'demo/demo.cc',
			'demo/speech_demo.cc',
			'demo/asr_demo.cc',
			'demo/tts_demo.cc',
			'demo/common.h',
		],
		'link_settings': {
			'libraries': [
				'-L<(deps_dir)/lib',
				'-lgrpc', '-Wl,-rpath,<(deps_dir)/lib',
			],
		},
	}, # target 'demo'
	]
}
