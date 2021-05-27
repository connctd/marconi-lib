#ifndef MARCONI_EVENTING_H_
#define MARCONI_EVENTING_H_

// can be used to pass a debug handler
typedef void debugCallback(const char *);

// calls error callback with error code
typedef void errorCallback(const unsigned char error);

class Eventing{
	public:
        Eventing(debugCallback *debug_callback, errorCallback *error_callback) {
			error_callback_ = error_callback;
			debug_callback_ = debug_callback;
		};

        // calls error_callback
        void error(unsigned char err);

        // call debug_callback with given message
        void debug(const char *debug_message);

        // wraps buffer in hex string
        void debugBytes(unsigned char *b, int len);

        void debugByte(unsigned char b); 

	private:
		debugCallback *debug_callback_;
		errorCallback *error_callback_;
};

#endif // MARCONI_EVENTING_H_