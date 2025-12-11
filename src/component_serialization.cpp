#include "component_serialization.hpp"
#include "components/gravity.hpp"
#include "components/inventory.hpp"
#include "components/item.hpp"
#include "components/jump.hpp"
#include "components/rigidbody.hpp"
#include "components/sprite.hpp"
#include "components/transform.hpp"
#include <SFML/Graphics/RectangleShape.hpp>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

ComponentSerializer &ComponentSerializer::Get() {
    static ComponentSerializer instance;
    return instance;
}

// Initialize custom serializers for complex types
void InitializeComponentSerializers() {
    auto &serializer = ComponentSerializer::Get();

    // Register sprite serializer (only serialize texture name, not the actual
    // texture/sprite object)
    serializer.RegisterSerializer<sprite>(
        ComponentID::Sprite,
        [](const sprite &spr) -> std::vector<uint8_t> {
            std::vector<uint8_t> data;
            const std::string &name = spr.texture_name;
            uint16_t nameLen = static_cast<uint16_t>(name.size());
            data.resize(sizeof(uint16_t) + nameLen);
            std::memcpy(data.data(), &nameLen, sizeof(uint16_t));
            std::memcpy(data.data() + sizeof(uint16_t), name.data(), nameLen);
            return data;
        },
        [](const uint8_t *data, size_t size) -> sprite {
            sprite spr;
            if (size >= sizeof(uint16_t)) {
                uint16_t nameLen;
                std::memcpy(&nameLen, data, sizeof(uint16_t));
                if (size >= sizeof(uint16_t) + nameLen) {
                    spr.texture_name =
                        std::string(reinterpret_cast<const char *>(data + sizeof(uint16_t)),
                                    nameLen);
                    // Note: Texture and sprite_obj will need to be loaded separately
                }
            }
            return spr;
        });

    // Register rigidbody serializer (don't serialize SFML hitbox, just dimensions
    // and properties)
    serializer.RegisterSerializer<rigidbody>(
        ComponentID::Rigidbody,
        [](const rigidbody &rb) -> std::vector<uint8_t> {
            std::vector<uint8_t> data;
            data.resize(sizeof(float) * 4 + sizeof(bool));
            size_t offset = 0;
            std::memcpy(data.data() + offset, rb.velocity, sizeof(float) * 2);
            offset += sizeof(float) * 2;
            std::memcpy(data.data() + offset, &rb.Mass, sizeof(float));
            offset += sizeof(float);
            std::memcpy(data.data() + offset, rb.base_size, sizeof(float) * 2);
            offset += sizeof(float) * 2;
            std::memcpy(data.data() + offset, &rb.can_collide, sizeof(bool));
            return data;
        },
        [](const uint8_t *data, size_t size) -> rigidbody {
            rigidbody rb;
            if (size >= sizeof(float) * 4 + sizeof(bool)) {
                size_t offset = 0;
                std::memcpy(rb.velocity, data + offset, sizeof(float) * 2);
                offset += sizeof(float) * 2;
                std::memcpy(&rb.Mass, data + offset, sizeof(float));
                offset += sizeof(float);
                std::memcpy(rb.base_size, data + offset, sizeof(float) * 2);
                offset += sizeof(float) * 2;
                std::memcpy(&rb.can_collide, data + offset, sizeof(bool));
                // Hitbox will be reconstructed from base_size
                rb.Hitbox = sf::RectangleShape({rb.base_size[0], rb.base_size[1]});
            }
            return rb;
        });

    // Register item serializer (skip ui_view sprite, just serialize properties)
    serializer.RegisterSerializer<item>(
        ComponentID::Item,
        [](const item &itm) -> std::vector<uint8_t> {
            std::vector<uint8_t> data;
            data.resize(sizeof(bool) * 2 + sizeof(float) * 2);
            size_t offset = 0;
            std::memcpy(data.data() + offset, &itm.is_picked_up, sizeof(bool));
            offset += sizeof(bool);
            std::memcpy(data.data() + offset, &itm.time_until_pickup, sizeof(float));
            offset += sizeof(float);
            std::memcpy(data.data() + offset, &itm.time_until_despawn, sizeof(float));
            offset += sizeof(float);
            std::memcpy(data.data() + offset, &itm.is_coin, sizeof(bool));
            return data;
        },
        [](const uint8_t *data, size_t size) -> item {
            item itm;
            if (size >= sizeof(bool) * 2 + sizeof(float) * 2) {
                size_t offset = 0;
                std::memcpy(&itm.is_picked_up, data + offset, sizeof(bool));
                offset += sizeof(bool);
                std::memcpy(&itm.time_until_pickup, data + offset, sizeof(float));
                offset += sizeof(float);
                std::memcpy(&itm.time_until_despawn, data + offset, sizeof(float));
                offset += sizeof(float);
                std::memcpy(&itm.is_coin, data + offset, sizeof(bool));
                // ui_view will need to be set separately
            }
            return itm;
        });

    // Register inventory serializer (skip entity references, just serialize
    // counts)
    serializer.RegisterSerializer<inventory>(
        ComponentID::Inventory,
        [](const inventory &inv) -> std::vector<uint8_t> {
            std::vector<uint8_t> data;
            data.resize(sizeof(int) * 3);
            size_t offset = 0;
            std::memcpy(data.data() + offset, &inv.coins, sizeof(int));
            offset += sizeof(int);
            std::memcpy(data.data() + offset, &inv.selected_slot, sizeof(int));
            offset += sizeof(int);
            std::memcpy(data.data() + offset, &inv.max_items, sizeof(int));
            return data;
        },
        [](const uint8_t *data, size_t size) -> inventory {
            inventory inv;
            if (size >= sizeof(int) * 3) {
                size_t offset = 0;
                std::memcpy(&inv.coins, data + offset, sizeof(int));
                offset += sizeof(int);
                std::memcpy(&inv.selected_slot, data + offset, sizeof(int));
                offset += sizeof(int);
                std::memcpy(&inv.max_items, data + offset, sizeof(int));
                // items vector will be empty initially
            }
            return inv;
        });
}
