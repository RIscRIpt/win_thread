#pragma once

#include <Windows.h>

#include <chrono>
#include <system_error>
#include <tuple>
#include <type_traits>

namespace win {

    namespace this_thread {

        inline void yield() noexcept {
            SwitchToThread();
        }

        DWORD get_id() noexcept {
            return GetCurrentThreadId();
        }

        template<typename Rep, typename Period>
        void sleep_for(std::chrono::duration<Rep, Period> const& sleep_duration) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(sleep_duration);
            if (duration.count() >= 0)
                Sleep(duration.count());
            else
                Sleep(0);
        }

        template<typename Clock, typename Duration>
        void sleep_until(std::chrono::time_point<Clock, Duration> const& wake_time) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(wake_time - std::chrono::system_clock::now());
            if (duration.count() >= 0)
                Sleep(duration.count());
            else
                Sleep(0);
        }

    }

    class thread {
    private:
        template<typename Fn, typename ...Ts>
        class wrapper {
        public:
            friend class ::win::thread;

            wrapper(Fn &&function, Ts&&... arguments)
                : function_(std::forward<Fn>(function))
                , arguments_(std::forward<Ts>(arguments)...)
            {}

        private:
            static DWORD __stdcall start_routine(LPVOID lpThreadParameter);

            Fn function_;
            std::tuple<Ts...> arguments_;
        };

    public:
        using id = DWORD;

        thread() noexcept
            : handle_(NULL)
        {}

        template<typename Fn, typename ...Ts>
        thread(Fn &&function, Ts&&... arguments) {
            auto w = new wrapper<Fn, Ts...>(std::forward<Fn>(function), std::forward<Ts>(arguments)...);
            handle_ = CreateThread(NULL, 0, w->start_routine, static_cast<LPVOID>(w), 0, NULL);
            if (!handle_) {
                delete w;
                throw std::system_error(GetLastError(), std::generic_category());
            }
        }

        ~thread() {
            if (joinable()) {
                TerminateThread(handle_, 0);
                CloseHandle(handle_);
                handle_ = NULL;
            }
        }

        thread(thread const&) = delete;

        thread(thread &&other) noexcept
            : thread()
        {
            swap(other);
        }

        thread& operator=(thread const&) = delete;

        thread& operator=(thread &&rhs) noexcept {
            swap(rhs);
            return *this;
        }

        void swap(thread &other) noexcept {
            std::swap(handle_, other.handle_);
        }

        bool joinable() const noexcept {
            return handle_ != NULL;
        }

        void join() {
            if (!joinable()) {
                throw std::system_error(std::make_error_code(std::errc::no_such_process));
            }

            if (get_id() == ::win::this_thread::get_id()) {
                throw std::system_error(std::make_error_code(std::errc::resource_deadlock_would_occur));
            }

            auto result = WaitForSingleObject(handle_, INFINITE);
            switch (result) {
                case WAIT_FAILED:
                    throw std::system_error(GetLastError(), std::generic_category());
                case WAIT_OBJECT_0:
                    detach();
                    return;
                case WAIT_ABANDONED:
                case WAIT_TIMEOUT:
                default:
                    throw std::runtime_error("thread: unexpected join result");
            }
        }

        void detach() {
            if (!joinable()) {
                throw std::system_error(std::make_error_code(std::errc::no_such_process));
            }

            CloseHandle(handle_);
            handle_ = NULL;
        }

        id get_id() const noexcept {
            return GetThreadId(handle_);
        }

        HANDLE native_handle() const noexcept {
            return handle_;
        }

        static unsigned int hardware_concurrency() noexcept {
            SYSTEM_INFO sys_info;
            GetNativeSystemInfo(&sys_info);
            return sys_info.dwNumberOfProcessors;
        }

    private:
        HANDLE handle_;
    };

    template<typename Fn, typename ...Ts>
    inline DWORD __stdcall thread::wrapper<Fn, Ts...>::start_routine(LPVOID lpThreadParameter) {
        std::unique_ptr<thread::wrapper<Fn, Ts...>> pw(reinterpret_cast<thread::wrapper<Fn, Ts...>*>(lpThreadParameter));
        if constexpr (std::is_same_v<std::invoke_result_t<Fn, Ts...>, void>) {
            std::apply(pw->function_, pw->arguments_);
            return 0;
        } else {
            return std::apply(t.function_, t.arguments_);
        }
    }

}
