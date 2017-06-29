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
		'target_name': 'speech',
		'type': 'shared_library',
		'sources': [
			'<(proto_gen_dir)/speech.pb.cc',
			'<(src_dir)/common/speech_connection.cc',
			'<(src_dir)/common/speech_connection.h',
			'<(src_dir)/common/speech_config.cc',
			'<(src_dir)/common/speech_config.h',
			'<(src_dir)/common/log.h',
			'<(src_dir)/common/log.cc',
			'<(src_dir)/common/pending_queue.h',
			'<(src_dir)/common/op_ctl.h',
			'<(src_dir)/tts/types.h',
			'<(src_dir)/tts/tts_impl.cc',
			'<(src_dir)/tts/tts_impl.h',
			'<(src_dir)/asr/types.h',
			'<(src_dir)/asr/asr_impl.cc',
			'<(src_dir)/asr/asr_impl.h',
			'<(src_dir)/speech/types.h',
			'<(src_dir)/speech/speech_impl.h',
			'<(src_dir)/speech/speech_impl.cc',
		],
		'include_dirs': [
			'include',
			'<(src_dir)/common',
			'<(proto_gen_dir)',
			'<(deps_dir)/include',
		],
		'direct_dependent_settings': {
			'include_dirs': [ 'include', '<(proto_gen_dir)', '<(deps_dir)/include', '<(src_dir)/common' ],
		},
		'link_settings': {
			'libraries': [
				'-L<(deps_dir)/lib', '-lprotobuf', '-Wl,-rpath,<(deps_dir)/lib',
				'-lPocoFoundation -lPocoNet -lPocoNetSSL'
			],
		},
	}, # target 'common'
	{
		'target_name': 'demo',
		'type': 'executable',
		'dependencies': [
			'speech',
		],
		'include_dirs': [
			'include',
			'src/common',
			'<(deps_dir)/include',
		],
		'sources': [
			'demo/demo.cc',
			'demo/tts_demo.cc',
			'demo/asr_demo.cc',
			'demo/speech_demo.cc',
		],
		'link_settings': {
			'libraries': [
				'-L<(deps_dir)/lib',
				'-lPocoFoundation -lPocoNet -lPocoNetSSL', '-Wl,-rpath,<(deps_dir)/lib',
			],
		},
	}, # target 'demo'
	],
}
