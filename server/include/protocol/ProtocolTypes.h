#ifndef PROTOCOL_TYPES_H
#define PROTOCOL_TYPES_H

#include <array>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>

namespace protocol{

    enum class MessageType {
        MENU_REQUEST,
        MENU_RESPONSE,
        ORDER_REQUEST,
        ORDER_ACCEPTED,
        ORDER_REFUSED,
        ORDER_READY,
        PROVIDER_IDENTIFY,
        DELIVERY,
        LOW_STOCK,
        HEARTBEAT,
        ERROR
    };

    enum class ErrorCode {
        INVALID_MESSAGE,
        MISSING_FIELD,
        INVALID_QUANTITY,
        UNKNOWN_PRODUCT,
        UNAUTHORIZED_OPERATION,
        INVALID_DELIVERY
    };

    enum class Product {
        coffee,
        tea,
        croissant
    };

    namespace detail {
        template <typename Enum>
        struct EnumEntry {
            Enum value;
            std::string_view name;
        };

        template <typename Enum>
        struct EnumTraits;

        template <>
        struct EnumTraits<MessageType> {
            static constexpr auto entries = std::to_array<EnumEntry<MessageType>>({
                {MessageType::MENU_REQUEST, "MENU_REQUEST"},
                {MessageType::MENU_RESPONSE, "MENU_RESPONSE"},
                {MessageType::ORDER_REQUEST, "ORDER_REQUEST"},
                {MessageType::ORDER_ACCEPTED, "ORDER_ACCEPTED"},
                {MessageType::ORDER_REFUSED, "ORDER_REFUSED"},
                {MessageType::ORDER_READY, "ORDER_READY"},
                {MessageType::PROVIDER_IDENTIFY, "PROVIDER_IDENTIFY"},
                {MessageType::DELIVERY, "DELIVERY"},
                {MessageType::LOW_STOCK, "LOW_STOCK"},
                {MessageType::HEARTBEAT, "HEARTBEAT"},
                {MessageType::ERROR, "ERROR"},
            });
        };

        template <>
        struct EnumTraits<ErrorCode> {
            static constexpr auto entries = std::to_array<EnumEntry<ErrorCode>>({
                {ErrorCode::INVALID_MESSAGE, "INVALID_MESSAGE"},
                {ErrorCode::MISSING_FIELD, "MISSING_FIELD"},
                {ErrorCode::INVALID_QUANTITY, "INVALID_QUANTITY"},
                {ErrorCode::UNKNOWN_PRODUCT, "UNKNOWN_PRODUCT"},
                {ErrorCode::UNAUTHORIZED_OPERATION, "UNAUTHORIZED_OPERATION"},
                {ErrorCode::INVALID_DELIVERY, "INVALID_DELIVERY"},
            });
        };

        template <>
        struct EnumTraits<Product> {
            static constexpr auto entries = std::to_array<EnumEntry<Product>>({
                {Product::coffee, "coffee"},
                {Product::tea, "tea"},
                {Product::croissant, "croissant"},
            });
        };
    }

    // Enum converters
    template <typename Enum>
    requires requires { detail::EnumTraits<Enum>::entries; }
    constexpr std::string_view to_string(Enum value) noexcept {
        for (const auto& entry : detail::EnumTraits<Enum>::entries) {
            if (entry.value == value) {
                return entry.name;
            }
        }
        return {};
    }

    template <typename Enum>
    requires requires { detail::EnumTraits<Enum>::entries; }
    constexpr std::optional<Enum> from_string(std::string_view sv) noexcept {
        for (const auto& entry : detail::EnumTraits<Enum>::entries) {
            if (entry.name == sv) {
                return entry.value;
            }
        }
        return std::nullopt;
    }

    // Shared data structure
    struct Items {
        int coffee{};
        int tea{};
        int croissant{};
    };

    // Reusable message structures
    struct EmptyMessage {
        MessageType type{};
    };

    struct ItemsMessage {
        MessageType type{};
        Items items{};
    };

    struct AcceptedMessage {
        MessageType type{};
        int orderId{};
        int preparationTime{};
    };

    struct RefusedMessage {
        MessageType type{};
        std::string reason{};
    };

    struct OrderIdMessage {
        MessageType type{};
        int orderId{};
    };

    struct LowStockMessage {
        MessageType type{};
        Product product{};
        int remaining{};
    };

    struct ErrorMessage {
        MessageType type{};
        ErrorCode code{};
        std::string message{};
    };

    // Protocol validation exception carrying the ErrorCode and descriptive message
    class ProtocolException : public std::runtime_error {
    public:
        ProtocolException(ErrorCode code, const std::string& message)
            : std::runtime_error(message), code_(code) {}

        ErrorCode code() const noexcept { return code_; }

    private:
        ErrorCode code_;
    };

    // Top-level message variant
    using AnyMessage = std::variant<
        EmptyMessage,
        ItemsMessage,
        AcceptedMessage,
        RefusedMessage,
        OrderIdMessage,
        LowStockMessage,
        ErrorMessage
    >;

}

#endif
