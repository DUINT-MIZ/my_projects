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

namespace cli_interactive {

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

    void clear() { write(STDIN_FILENO, clearing_msg, clearing_msg_len); }
}

namespace termios_local {
    static constexpr tcflag_t kEchoInput     = ECHO;
    static constexpr tcflag_t kEchoBackspace = ECHOE;
    static constexpr tcflag_t kEchoNewline   = ECHONL;
    static constexpr tcflag_t kEchoLineKill  = ECHOK;
    static constexpr tcflag_t kAcceptSignals = ISIG;
    static constexpr tcflag_t kCanonicalMode = ICANON;

}

namespace termios_cc {
    namespace canonical {
        static constexpr int kEOFchar   = VEOF;
        static constexpr int kEOLchar   = VEOL; // end of line
        static constexpr int kEraseChar = VERASE;
        static constexpr int kKillChar  = VKILL;
    }

    namespace non_canonical {
        static constexpr int kReadMinChar = VMIN;
        static constexpr int kTimeoutTime = VTIME;
    }

    static constexpr int kInterruptChar = VINTR;
    static constexpr int kQuitChar      = VQUIT;
    static constexpr int kSuspendChar   = VSUSP;
}

namespace setup_action {
    static constexpr int kNow = TCSANOW; // Set terminal settings now
    static constexpr int kOutputDrained = TCSADRAIN; // Wait for output buffer to be drained
    static constexpr int kIOBuffEmpty = TCSAFLUSH; // Wait for output buffer to be drained + Flush Input Buffer
}

namespace detail {
    enum class SettingStatus {
        APPLYING,
        APPLIED,
        RESETING,
        NONE
    };
}

class TerminalSettings {
    private :
    using SettingStatus = detail::SettingStatus;
    struct termios old_settings;
    struct termios new_settings;
    SettingStatus setting_status;

    public :

    TerminalSettings() {
        tcgetattr(STDIN_FILENO, &old_settings);
        if(errno == ENOTTY)
            throw std::system_error(errno, std::generic_category(), "stdin is not a typewriter (Maybe program was piped ?)");
        new_settings = old_settings;
    }
    
    TerminalSettings(const TerminalSettings& oth) = delete;
    TerminalSettings(TerminalSettings&& oth) = delete;
    TerminalSettings& operator=(const TerminalSettings& oth) = delete;
    TerminalSettings& operator=(TerminalSettings&& oth) = delete;

    /* apply new setting */
    void apply(int action);

    /* apply old setting */
    void reset(int action);

    void setting_reset() noexcept {
        new_settings = old_settings;
    }

    tcflag_t& local_flag() noexcept { return new_settings.c_lflag; }
    const cc_t* get_cc() const noexcept { return new_settings.c_cc;}

    static void deactivate(tcflag_t& target_flag, tcflag_t mask) noexcept { target_flag &= ~(mask); }
    static void activate(tcflag_t& target_flag, tcflag_t mask) noexcept { target_flag |= mask; }

    /* This function and get_cc() purpose is to prevent
    user from modifying &new_settings.c_cc[0], which is used to access the array itself
    */
    void control_char(int index, cc_t value) noexcept {
        new_settings.c_cc[index] = value;
    }


    SettingStatus get_setting_status() const noexcept { return setting_status; }
};

namespace detail {

    class {
        using SettingStatus = detail::SettingStatus;
        private :
        bool setted_up = false;
        public :
        
        void update(TerminalSettings& setter) {
            switch (setter.get_setting_status())
            {
            case SettingStatus::APPLYING :
                if(setted_up) { 
                    throw std::runtime_error("Another terminal settings has been applied before");
                }
                break;
                
            case SettingStatus::RESETING :
                if(setted_up) setted_up = false;
                break;
            }
        }

    } obj;
}

void TerminalSettings::reset(int action) {
        setting_status = SettingStatus::RESETING ;
        detail::obj.update(*this);
        errno = 0;
        tcsetattr(STDIN_FILENO, action, &old_settings);
        if(errno != 0)
            throw std::system_error(errno, std::generic_category());
}

void TerminalSettings::apply(int action) {
    setting_status = SettingStatus::APPLYING;
    detail::obj.update(*this);
    errno = 0;
    tcsetattr(STDIN_FILENO, action, &new_settings);
    if(errno != 0)
        throw std::system_error(errno, std::generic_category());
}




}
using namespace cli_interactive;
int main() {
    TerminalSettings setter;
    setter.deactivate(
        setter.local_flag(),
        termios_local::kCanonicalMode |
        termios_local::kEchoInput
    );
    setter.control_char(
        termios_cc::non_canonical::kReadMinChar,
        1
    );

    setter.apply(setup_action::kNow);

    
    
    setter.reset(setup_action::kNow);
}