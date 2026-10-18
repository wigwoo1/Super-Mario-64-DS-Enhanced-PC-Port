#ifndef PC_MEMBER_CALLBACK_H
#define PC_MEMBER_CALLBACK_H

#include <cstdint>
#include <cstring>

// Recovered DS tables always reserve two words per member callback. MSVC's
// single-inheritance pointer is only one word: keep storage and calls separate.
template<class R, class... Args>
struct PcMemberCallback {
    std::uintptr_t address;
    std::int32_t adjustment;

    template<class T>
    PcMemberCallback &operator=(R (T::*method)(Args...)) {
        static_assert(sizeof(method) == sizeof(address),
                      "native callback adapter requires single inheritance");
        std::memcpy(&address, &method, sizeof(address));
        adjustment = 0;
        return *this;
    }

    struct Bound {
        void *self;
        std::uintptr_t address;
        std::int32_t adjustment;
        R operator()(Args... args) const {
            using Function = R (__thiscall *)(void *, Args...);
            auto object = static_cast<char *>(self) + adjustment;
            return reinterpret_cast<Function>(address)(object, args...);
        }
    };
};

template<class T, class R, class... Args>
typename PcMemberCallback<R, Args...>::Bound
operator->*(T *self, PcMemberCallback<R, Args...> callback) {
    return {self, callback.address, callback.adjustment};
}

static_assert(sizeof(PcMemberCallback<int>) == 8, "recovered callback stride");
#endif
