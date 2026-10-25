#pragma once

#include <unknwn.h>

namespace FrameDirector {

/// Minimal COM smart pointer (works with both MSVC and MinGW, no WRL needed).
template <typename T>
class ComPtr {
public:
	ComPtr() = default;
	ComPtr(const ComPtr &p_other) : ptr(p_other.ptr) { if (ptr) ptr->AddRef(); }
	ComPtr &operator=(const ComPtr &p_other) {
		if (this != &p_other) {
			reset();
			ptr = p_other.ptr;
			if (ptr) ptr->AddRef();
		}
		return *this;
	}
	~ComPtr() { reset(); }

	T *get() const { return ptr; }
	T *operator->() const { return ptr; }
	explicit operator bool() const { return ptr != nullptr; }
	T **put() {
		reset();
		return &ptr;
	}
	void reset() {
		if (ptr) {
			ptr->Release();
			ptr = nullptr;
		}
	}
	template <typename U>
	HRESULT as(ComPtr<U> &r_out) const {
		return ptr ? ptr->QueryInterface(__uuidof(U), reinterpret_cast<void **>(r_out.put())) : E_POINTER;
	}

private:
	T *ptr = nullptr;
};

} // namespace FrameDirector
