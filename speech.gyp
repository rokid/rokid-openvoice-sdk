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
			'libraries': [ '-L<(deps_dir)/lib', '-lgrpc++', '-lprotobuf', '-Wl,-rpath,<(deps_dir)/lib' ],
		},
	}, # target 'common'
	{
		'target_name': 'tts',
		'type': 'shared_library',
		'dependencies': [
			'common',
		],
		'sources': [
			'<(src_dir)/common/pending_queue.h',
			'<(src_dir)/common/grpc_runner.h',
			'<(src_dir)/common/callback.h',
			'<(src_dir)/tts/tts_runner.cc',
			'<(src_dir)/tts/tts_runner.h',
			'<(src_dir)/tts/tts.cc',
			'<(src_dir)/tts/tts.h',
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
			'<(src_dir)/common/grpc_runner.h',
			'<(src_dir)/common/callback.h',
			'<(src_dir)/asr/asr_runner.h',
			'<(src_dir)/asr/asr_runner.cc',
			'<(src_dir)/asr/asr.h',
			'<(src_dir)/asr/asr.cc',
		],
	}, # target 'asr'
	{
		'target_name': 'nlp',
		'type': 'shared_library',
		'dependencies': [
			'common',
		],
		'sources': [
			'<(src_dir)/common/pending_queue.h',
			'<(src_dir)/common/grpc_runner.h',
			'<(src_dir)/common/callback.h',
			'<(src_dir)/nlp/nlp_runner.h',
			'<(src_dir)/nlp/nlp_runner.cc',
			'<(src_dir)/nlp/nlp.h',
			'<(src_dir)/nlp/nlp.cc',
		],
	}, # target 'nlp'
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
			'<(src_dir)/speech/core.h',
			'<(src_dir)/speech/core.cc',
			'<(src_dir)/speech/voice_stream_reader.cc',
			'<(src_dir)/speech/voice_stream_reader.h',
		],
	}, # target 'speech'
	{
		'target_name': 'demo',
		'type': 'executable',
		'include_dirs': [
			'<(src_dir)/common',
			'<(src_dir)/tts',
			'<(src_dir)/asr',
			'<(src_dir)/nlp',
			'<(proto_gen_dir)',
			'<(deps_dir)/include',
			'include',
		],
		'dependencies': [
			'tts',
			'asr',
			'nlp',
			'speech',
		],
		'sources': [
			'<(test_dir)/demo.cc',
		],
		'link_settings': {
			'libraries': [ '-Wl,-rpath-link,<(deps_dir)/lib' ],
			'ldflags': [ '-Wl,--copy-dt-needed-entries' ],
		},
	}
	]
}
