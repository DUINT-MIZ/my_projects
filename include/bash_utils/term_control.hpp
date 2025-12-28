#include <vector>
#include <unistd.h>
#include <termios.h>
#include <stdexcept>
#include <iostream>
#include <system_error>

/*
A library to make terminal more interactive in some way
still in early version
*/

namespace term_control {

namespace helpers {
    constexpr size_t string_length(const char* str) noexcept {
        size_t res = 0;
        while(*(str++) != '\0') ++res;
        return res;
    }
}

namespace effects {
    static constexpr const char* clearing_msg = "\033[2J\033[3J\033[H";
    static constexpr size_t clearing_msg_len = helpers::string_length(clearing_msg);

    void clear() { write(STDOUT_FILENO, clearing_msg, clearing_msg_len); }
}

template <typename T, typename Signature>
struct value_signaturing {
    protected :
    T val;
    
    public :
    constexpr value_signaturing(const T& oth_val) : val(oth_val) {}
    constexpr value_signaturing(T&& oth_val) : val(std::move(oth_val)) {}
    
    constexpr value_signaturing<T, Signature>& operator=(const T& oth_val) {
        this->val = oth_val;
        return *this;
    }

    constexpr value_signaturing<T, Signature>& operator=(T&& oth_val) {
        this->val = std::move(oth_val);
        return *this;
    }

    constexpr T get() const noexcept { return this->val; }
};

template <typename T, typename Signature>
struct flagging : public value_signaturing<T, Signature> {
    
    public :

    
    constexpr flagging(const T& oth_val) : value_signaturing<T, Signature>(oth_val) {}
    constexpr flagging(T&& oth_val) : value_signaturing<T, Signature>(std::move(oth_val)) {}

    
    constexpr flagging<T, Signature>& operator=(const T& oth_val) {
        value_signaturing<T, Signature>::operator=(oth_val);
        return *this;
    }

    constexpr flagging<T, Signature>& operator=(T&& oth_val) {
        value_signaturing<T, Signature>::operator=(std::move(oth_val));
        return *this;
    }

    
    constexpr flagging<T, Signature> operator|(const flagging<T, Signature>& oth_val) const {
        return flagging<T, Signature>(this->val | oth_val.get());
    }

    
    constexpr flagging<T, Signature>& operator|=(const flagging<T, Signature>& oth_val) {
        this->val |= oth_val.get(); 
        return *this;
    }
};

static constexpr bool activate = true;
static constexpr bool deactivate = false;

namespace termios_local {
    struct TermiosLocalSig {};
    using TermLocalType = flagging<tcflag_t, TermiosLocalSig>;
    static constexpr TermLocalType echoInput = ECHO;
    static constexpr TermLocalType EchoBackspace  = ECHOE;
    static constexpr TermLocalType EchoNewline    = ECHONL;
    static constexpr TermLocalType EchoLineKill   = ECHOK;
    static constexpr TermLocalType AcceptSignals  = ISIG;
    static constexpr TermLocalType CanonicalMode  = ICANON;
}

namespace termios_cc {
    struct TermiosCCSig {};
    using TermCCType = value_signaturing<int, TermiosCCSig>;

    namespace canonical {
        static constexpr TermCCType EOFchar   = VEOF;
        static constexpr TermCCType EOLchar   = VEOL; // end of line
        static constexpr TermCCType EraseChar = VERASE;
        static constexpr TermCCType KillChar  = VKILL;
    }

    namespace non_canonical {
        static constexpr TermCCType ReadMinChar = VMIN;
        static constexpr TermCCType TimeoutTime = VTIME;
    }

    static constexpr TermCCType InterruptChar = VINTR;
    static constexpr TermCCType QuitChar      = VQUIT;
    static constexpr TermCCType SuspendChar   = VSUSP;
}

namespace commit_action {
    struct CommitActSig {};
    using CommitActType = value_signaturing<int, CommitActSig>;
    static constexpr CommitActType Now = TCSANOW; // Set terminal settings now
    static constexpr CommitActType OutputDrained = TCSADRAIN; // Wait for output buffer to be drained
    static constexpr CommitActType IOBuffEmpty = TCSAFLUSH; // Wait for output buffer to be drained + Flush Input Buffer
}

class TerminalSettings {
    private :
    struct termios old_settings;
    struct termios new_settings;
    commit_action::CommitActType reset_action = commit_action::IOBuffEmpty;

    public :

    TerminalSettings() {
        tcgetattr(STDIN_FILENO, &old_settings);
        if(errno == ENOTTY)
            throw std::system_error(errno, std::generic_category(), "stdin is not a teletype (Maybe program was piped ?)");
        new_settings = old_settings;
    }
    
    TerminalSettings(const TerminalSettings& oth) = delete;
    TerminalSettings(TerminalSettings&& oth) = delete;
    TerminalSettings& operator=(const TerminalSettings& oth) = delete;
    TerminalSettings& operator=(TerminalSettings&& oth) = delete;

    void set_local_flag(const termios_local::TermLocalType& flag, bool enable) noexcept {
        if(enable) {
            new_settings.c_lflag |= flag.get();
        } else {
            new_settings.c_lflag &= ~(flag.get());
        }
    }
    
    void set_control_char(const termios_cc::TermCCType& cc_index, cc_t val) noexcept {
        new_settings.c_cc[cc_index.get()] = val;
    }

    void reset_current() noexcept { new_settings = old_settings; }

    void commit(const commit_action::CommitActType& action) const {
        errno = 0;
        tcsetattr(STDIN_FILENO, action.get(), &new_settings);
        if(errno != 0)
            throw std::system_error(errno, std::generic_category(), ", terminal setting commit failed...");
    }

    const tcflag_t& get_local_flag() const noexcept { return new_settings.c_lflag; }
    const cc_t* get_control_char() const noexcept { return new_settings.c_cc; }

    ~TerminalSettings() { tcsetattr(STDIN_FILENO, reset_action.get(), &old_settings); }
};
}