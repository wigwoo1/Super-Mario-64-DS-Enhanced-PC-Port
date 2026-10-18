#pragma once
#include <cstddef>
#include <type_traits>

// Cartridge state handlers copy two words. Keep that storage contract, but
// populate the first word with a native C thunk, never an ARM code address.
template<class... Args> struct PcMinigameCallback {
    using Function = void (__cdecl *)(void *, Args...);
    Function function;
    int adjustment;
    constexpr PcMinigameCallback(std::nullptr_t = nullptr) : function(nullptr), adjustment(0) {}
    constexpr PcMinigameCallback(Function fn, int delta = 0) : function(fn), adjustment(delta) {}
    explicit operator bool() const { return function != nullptr; }
    bool operator==(std::nullptr_t) const { return !function; }
    bool operator!=(std::nullptr_t) const { return function != nullptr; }
    void operator()(void *self, Args... args) const {
        if (function) function(static_cast<char *>(self) + adjustment, args...);
    }
};
static_assert(sizeof(PcMinigameCallback<>) == 8, "minigame callback record");
static_assert(sizeof(PcMinigameCallback<int>) == 8, "minigame slot callback record");
static_assert(std::is_trivially_copyable<PcMinigameCallback<>>::value, "two-word callback copies");
