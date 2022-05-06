namespace cw {

/**
 * \brief Lightweight optional wrapper using one special value as empty designator
 * 
 * - simpler and faster than std::optional
 * - no additional storage space required  
 * - removes one value of type from valid range
 * - empty designator and check customizable via Policy structs (\ref policies)
 * 
 * \see policies
 * 
 * \section usage Usage example
 * 
 * Use \ref policy_optional::policy_optional::has_value to check if value is set and \ref policy_optional::policy_optional::get to access it.
 * 
 * \code
 * // create optional using maximum (255) as empty case
 * cw::policy_optional::max_optional<unsigned char> opt = 1;
 * // check if value exists and then print it
 * if (opt.has_value()) std::cout << opt.get() << std::endl;
 * // reset to empty value
 * opt.reset();
 * \endcode
 */
namespace policy_optional {

/**
 * \brief Optional value
 * 
 * Optional wrapper for a type using one special sentinel value to mark as empty.
 * 
 * \tparam T Type to store
 * \tparam Policy Strategy pattern for which value is empty sentinel and how to check for empty
 */
template <typename T, typename Policy>
class policy_optional {
private:
   T val;
public:
    /**
     * \brief Type wrapped with optional
     */
    using value_type = T;

    /**
     * \brief Policy used for optional
     */
    using policy_type = Policy;

    /**
     * \brief Construct empty optional
     * 
     * Uses empty value of policy.
     */
    constexpr policy_optional(): val(Policy::empty_value()) {}
    /**
     * \brief Construct filled optional
     * 
     * \param t Value to wrap
     */
    constexpr policy_optional(T t): val(std::move(t)) {}

    /**
     * \brief Check if value is given and not empty sentinel
     * 
     * \return Wether value is given
     */
    constexpr inline bool has_value() const noexcept { return !Policy::is_empty_value(val); }

    /**
     * \brief Reset to empty sentinel
     */
    constexpr inline void reset() noexcept { val = Policy::empty_value(); }

    /**
     * \brief Unchecked value access
     */
    constexpr inline T const& get() const& noexcept { return val; }

    /**
     * \brief Unchecked value access
     */
    constexpr inline T& get() & noexcept { return val; }

    /**
     * \brief Unchecked value access
     */
    constexpr inline T&& get() && noexcept { return std::move(val); }

    /**
     * \brief Construct type inside optional wrapper
     * 
     * \tparam Args constructor argument types
     * \param args constructor arguments
     */
    template <typename... Args>
    constexpr inline void emplace(Args&&... args) { val = T(std::forward<Args>(args)...); }

    /**
     * \brief Swap internal value
     * 
     * \param lhs value to swap
     * \param rhs value to swap
     */
    friend void swap(policy_optional& lhs, policy_optional& rhs) { std::swap(lhs.val, rhs.val); }
};

/**
 * \defgroup policies Policies for policy_optional 
 * \brief Strategy structs that specify empty value and empty check for \ref policy_optional
 */

/**
 * \brief Optional strategy using maximum as sentinel
 * 
 * Strategy for \ref policy_optional to use the maximum number of the type as it's empty sentinel value.
 * 
 * \tparam T Numeric type with a maximum value
 * \ingroup policies
 */
template <typename T>
struct max_policy
{
    static constexpr T empty_value() { return std::numeric_limits<T>::max(); }
    static constexpr bool is_empty_value(T v) { return v == std::numeric_limits<T>::max(); }
};

/**
 * \brief Optional strategy using maximum as sentinel
 * 
 * Strategy for \ref policy_optional to use empty string with two null terminators and length > 1 as special sentinel.
 * \ingroup policies
 */
struct string_policy
{
    static std::string empty_value() { return std::string("\0\0", 2); }
    static bool is_empty_value(const std::string& v) { return v.compare(0, v.npos, "\0\0", 2) == 0; }
};

/**
 * \brief Optional type wrapper using max sentinel
 * 
 * Uses \ref max_policy and maximum number as empty case.
 * 
 * \tparam T Numeric type with possible max value
 */
template <typename T>
using max_optional = policy_optional<T, max_policy<T>>;

/**
 * \brief Optional string wrapper
 * 
 * Uses \ref string_policy with two null terminator length > 1 empty string as sentinel.
 */
using string_optional = policy_optional<std::string, string_policy>;

}
}
