
#include <assert.h>
#include <stddef.h>
#include <type_traits>
class Fmt {
public:

    template<typename T>
    Fmt(const char* fmt, T val) {
        // static_assert 确保传入的是算术类型（int, float 等），不能传 string 对象
        static_assert(std::is_arithmetic<T>::value == true, "Must be arithmetic type");
        
        length_ = snprintf(buf_, sizeof(buf_), fmt, val);
        assert(static_cast<size_t>(length_) < sizeof(buf_));
    }

    const char* data() const { return buf_; }
    int length() const { return length_; }

private:
    char buf_[32];
    int length_;
};

