#define DISABLE_COPY(CLASS) \
	CLASS(const CLASS&) = delete; \
	CLASS& operator=(const CLASS&) = delete;
