#ifndef PROTOCOL_JSON_H
#define PROTOCOL_JSON_H

#include <protocol/ProtocolTypes.h>
#include <nlohmann/json.hpp>
#include <initializer_list>
#include <string>
#include <string_view>

namespace protocol{

    // JSON helper serialisers / deserialisers
    using json = nlohmann::json;

    namespace detail {
        template <typename Enum>
        inline void enum_to_json(json& j, Enum value, ErrorCode err_code, std::string_view label) {
            const auto sv = to_string(value);
            if (sv.empty()) {
                throw ProtocolException(err_code, "Invalid " + std::string(label) + " value");
            }
            j = sv;
        }

        template <typename Enum>
        inline void enum_from_json(const json& j,
                                   Enum& value,
                                   ErrorCode type_err,
                                   ErrorCode value_err,
                                   std::string_view label) {
            if (!j.is_string()) {
                throw ProtocolException(type_err, std::string(label) + " must be a string");
            }
            const auto& str = j.get_ref<const std::string&>();
            const auto parsed = from_string<Enum>(str);
            if (!parsed) {
                throw ProtocolException(value_err, "Unknown " + std::string(label) + ": " + str);
            }
            value = *parsed;
        }

        void validate_object_keys(const json& j,
                                  std::initializer_list<std::string_view> required,
                                  std::initializer_list<std::string_view> allowed);

        int parse_int(const json& j,
                      std::string_view field_name,
                      int min_val,
                      int max_val,
                      ErrorCode err_code);

        std::string parse_string_field(const json& j, std::string_view field_name);

        void validate_message_type(MessageType actual,
                                   std::initializer_list<MessageType> expected,
                                   std::string_view struct_name);

        bool has_positive_item(const Items& items) noexcept;
    }

    // Enum JSON converters
    void to_json(json& j, MessageType type);
    void from_json(const json& j, MessageType& type);

    void to_json(json& j, ErrorCode code);
    void from_json(const json& j, ErrorCode& code);

    void to_json(json& j, Product product);
    void from_json(const json& j, Product& product);

    // Items
    void to_json(json& j, const Items& items);
    void from_json(const json& j, Items& items);

    // EmptyMessage (MENU_REQUEST, PROVIDER_IDENTIFY, HEARTBEAT)
    void to_json(json& j, const EmptyMessage& msg);
    void from_json(const json& j, EmptyMessage& msg);

    // ItemsMessage (ORDER_REQUEST, MENU_RESPONSE, DELIVERY)
    void to_json(json& j, const ItemsMessage& msg);
    void from_json(const json& j, ItemsMessage& msg);

    // AcceptedMessage (ORDER_ACCEPTED)
    void to_json(json& j, const AcceptedMessage& msg);
    void from_json(const json& j, AcceptedMessage& msg);

    // RefusedMessage (ORDER_REFUSED)
    void to_json(json& j, const RefusedMessage& msg);
    void from_json(const json& j, RefusedMessage& msg);

    // OrderIdMessage (ORDER_READY)
    void to_json(json& j, const OrderIdMessage& msg);
    void from_json(const json& j, OrderIdMessage& msg);

    // LowStockMessage (LOW_STOCK)
    void to_json(json& j, const LowStockMessage& msg);
    void from_json(const json& j, LowStockMessage& msg);

    // ErrorMessage (ERROR)
    void to_json(json& j, const ErrorMessage& msg);
    void from_json(const json& j, ErrorMessage& msg);

    // Top-level message type inspection & variant dispatch
    MessageType parse_message_type(const json& j);
    AnyMessage deserialize_message(const json& j);

}

#endif
