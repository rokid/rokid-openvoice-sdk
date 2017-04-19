#pragma once

#include <stdint.h>
#include <memory>

template <typename OutType>
class PipelineOutHandler {
public:
	virtual std::shared_ptr<OutType> poll() = 0;

	virtual bool closed() = 0;
};

class PipelineHandlerBase {
public:
	PipelineHandlerBase() : error_code_(0) {
	}

	virtual ~PipelineHandlerBase() {}

	virtual int32_t do_work(void* arg) = 0;

	// error_code 0  no error
	//            1  pipeline not ready
	//            2  input is closed
	//            3  unknown
	//           <0  custom error codes
	inline int32_t error_code() { return error_code_; }

protected:
	int32_t error_code_;
};

#define FLAG_BREAK_LOOP 0x1
#define FLAG_AS_HEAD 0x2
#define FLAG_NOT_POLL_NEXT 0x4
#define FLAG_ERROR 0x8
template <typename InType, typename OutType>
class PipelineHandler : public PipelineOutHandler<OutType>
                      , public PipelineHandlerBase {
public:
	PipelineHandler() : input_(NULL), last_handle_ret_(0) {
	}

	virtual ~PipelineHandler() {}

	// FLAG_AS_HEAD: next loop should begin from this handler
	// FLAG_BREAK_LOOP: break current loop
	// FLAG_NOT_POLL_NEXT: use current polled data in next loop
	// FLAG_ERROR: failed, see error_code_
	int32_t do_work(void* arg) {
		int32_t r;
		std::shared_ptr<InType> in_data;

		error_code_ = 0;
		if (input_ == NULL) {
			error_code_ = 1;
			return FLAG_ERROR;
		}

		if ((last_handle_ret_ & FLAG_NOT_POLL_NEXT) == 0) {
			in_data = input_->poll();
			if (input_->closed()) {
				error_code_ = 2;
				return FLAG_ERROR;
			}
			start_handle(in_data, arg);
		} else
			in_data = last_data_;

		r = handle(in_data, arg);
		if (r & FLAG_ERROR) {
			if (error_code_ == 0)
				error_code_ = 3;
			goto exit;
		}
		if (r & FLAG_NOT_POLL_NEXT)
			last_data_ = in_data;

exit:
		if ((r & FLAG_NOT_POLL_NEXT) == 0) {
			end_handle(in_data, arg);
			last_data_.reset();
		}
		last_handle_ret_ = r;
		return last_handle_ret_;
	}

	inline void set_input(PipelineOutHandler<InType>* input) {
		input_ = input;
	}

protected:
	virtual void start_handle(std::shared_ptr<InType> in, void* arg) = 0;

	// return value:  1  'out' is valid data, but 'in' not handled finish, need call 'handle' with the 'in' again
	//                0  'out' is valid data, 'in' is handled finish
	//               -1  error occurs
	virtual int32_t handle(std::shared_ptr<InType> in, void* arg) = 0;

	virtual void end_handle(std::shared_ptr<InType> in, void* arg) = 0;

protected:
	PipelineOutHandler<InType>* input_;
	std::shared_ptr<InType> last_data_;
	int32_t last_handle_ret_;
};
