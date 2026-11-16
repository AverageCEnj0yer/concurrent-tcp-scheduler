#include <protocol/ProtocolJson.h>

#include <cstdint>
#include <limits>
#include <utility>

namespace protocol{

    namespace detail {
        void validate_object_keys(const json& j,
                                  std::initializer_list<std::string_view> required,
                                  std::initializer_list<std::string_view> allowed) {
            if (!j.is_object()) {
                throw ProtocolException(ErrorCode::INVALID_MESSAGE, "Message must be a JSON object");
            }
            for (const auto req : required) {
                if (!j.contains(req)) {
                    throw ProtocolException(ErrorCode::MISSING_FIELD,
                        "Missing required field: " + std::string(req));
                }
            }
            for (const auto& item : j.items()) {
                bool is_allowed = false;
                for (const auto ok : allowed) {
                    if (item.key() == ok) {
                        is_allowed = true;
                        break;
                    }
                }
                if (!is_allowed) {
                    throw ProtocolException(ErrorCode::INVALID_MESSAGE,
                        "Unknown field: " + item.key());
                }
            }
        }

        int parse_int(const json& j,
                      std::string_view field_name,
                      int min_val,
                      int max_val,
                      ErrorCode err_code) {
            if (j.is_number_unsigned()) {
                const auto val = j.get<std::uint64_t>();
                if ((min_val > 0 && val < static_cast<std::uint64_t>(min_val)) ||
                    max_val < 0 || val > static_cast<std::uint64_t>(max_val)) {
                    throw ProtocolException(err_code,
                        "Field '" + std::string(field_name) + "' must be an integer between " +
                        std::to_string(min_val) + " and " + std::to_string(max_val));
                }
                return static_cast<int>(val);
            }
            if (j.is_number_integer()) {
                const auto val = j.get<std::int64_t>();
                if (val < min_val || val > max_val) {
                    throw ProtocolException(err_code,
                        "Field '" + std::string(field_name) + "' must be an integer between " +
                        std::to_string(min_val) + " and " + std::to_string(max_val));
                }
                return static_cast<int>(val);
            }
            throw ProtocolException(err_code,
                "Field '" + std::string(field_name) + "' must be an integer");
        }

        std::string parse_string_field(const json& j, std::string_view field_name) {
            const auto& val = j.at(field_name);
            if (!val.is_string()) {
                throw ProtocolException(ErrorCode::INVALID_MESSAGE,
                    "Field '" + std::string(field_name) + "' must be a string");
            }
            return val.get<std::string>();
        }

        void validate_message_type(MessageType actual,
                                   std::initializer_list<MessageType> expected,
                                   std::string_view struct_name) {
            for (const auto candidate : expected) {
                if (actual == candidate) {
                    return;
                }
            }
            const auto name = to_string(actual);
            throw ProtocolException(ErrorCode::INVALID_MESSAGE,
                "Invalid message type '" +
                std::string(name.empty() ? "<unknown>" : name) +
                "' for " + std::string(struct_name));
        }

        bool has_positive_item(const Items& items) noexcept {
            return items.coffee > 0 || items.tea > 0 || items.croissant > 0;
        }
    }

    // Enum JSON converters
    void to_json(json& j, MessageType type) {
        detail::enum_to_json(j, type, ErrorCode::INVALID_MESSAGE, "MessageType");
    }

    void from_json(const json& j, MessageType& type) {
        detail::enum_from_json(j, type, ErrorCode::INVALID_MESSAGE, ErrorCode::INVALID_MESSAGE, "MessageType");
    }

    void to_json(json& j, ErrorCode code) {
        detail::enum_to_json(j, code, ErrorCode::INVALID_MESSAGE, "ErrorCode");
    }

    void from_json(const json& j, ErrorCode& code) {
        detail::enum_from_json(j, code, ErrorCode::INVALID_MESSAGE, ErrorCode::INVALID_MESSAGE, "ErrorCode");
    }

    void to_json(json& j, Product product) {
        detail::enum_to_json(j, product, ErrorCode::UNKNOWN_PRODUCT, "Product");
    }

    void from_json(const json& j, Product& product) {
        detail::enum_from_json(j, product, ErrorCode::INVALID_MESSAGE, ErrorCode::UNKNOWN_PRODUCT, "Product");
    }

    // Items
    void to_json(json& j, const Items& items) {
        if (items.coffee < 0 || items.tea < 0 || items.croissant < 0) {
            throw ProtocolException(ErrorCode::INVALID_QUANTITY, "Item quantities cannot be negative");
        }
        j = json{
            {"coffee", items.coffee},
            {"tea", items.tea},
            {"croissant", items.croissant}
        };
    }

    void from_json(const json& j, Items& items) {
        if (!j.is_object()) {
            throw ProtocolException(ErrorCode::INVALID_MESSAGE, "Field 'items' must be a JSON object");
        }
        Items parsed{};
        constexpr int max_qty = std::numeric_limits<int>::max();
        for (const auto& entry : j.items()) {
            const auto& key = entry.key();
            if (key == "coffee") {
                parsed.coffee = detail::parse_int(entry.value(), "coffee", 0, max_qty, ErrorCode::INVALID_QUANTITY);
            } else if (key == "tea") {
                parsed.tea = detail::parse_int(entry.value(), "tea", 0, max_qty, ErrorCode::INVALID_QUANTITY);
            } else if (key == "croissant") {
                parsed.croissant = detail::parse_int(entry.value(), "croissant", 0, max_qty, ErrorCode::INVALID_QUANTITY);
            } else {
                throw ProtocolException(ErrorCode::UNKNOWN_PRODUCT, "Unknown product: " + key);
            }
        }
        items = parsed;
    }

}
