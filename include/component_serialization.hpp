#pragma once

#include <cstdint>
#include <cstring>
#include <functional>
#include <map>
#include <string>
#include <vector>

// Component IDs for serialization
enum class ComponentID : uint8_t {
    Transform = 1,
    Rigidbody = 2,
    Sprite = 3,
    Gravity = 4,
    Jump = 5,
    Inventory = 6,
    Item = 7,
    Player = 8,
    EntityState = 9,
    // Add more as needed
};

// Serialization result
struct SerializedComponent {
    ComponentID id;
    std::vector<uint8_t> data;
};

// Component Serializer class
class ComponentSerializer {
  public:
    static ComponentSerializer &Get();

    // Serialize a component to bytes
    template <typename T>
    SerializedComponent Serialize(const T &component, ComponentID id);

    // Deserialize a component from bytes
    template <typename T>
    T Deserialize(const uint8_t *data, size_t size);

    // Register a custom serializer for complex types
    template <typename T>
    void RegisterSerializer(
        ComponentID id,
        std::function<std::vector<uint8_t>(const T &)> serializer,
        std::function<T(const uint8_t *, size_t)> deserializer);

    // Check if a custom serializer exists for a type
    template <typename T>
    bool HasCustomSerializer() const;

    // Serialize component with custom serializer
    template <typename T>
    SerializedComponent SerializeCustom(const T &component);

    // Deserialize component with custom serializer
    template <typename T>
    T DeserializeCustom(const uint8_t *data, size_t size);

  private:
    ComponentSerializer() = default;

    // Storage for custom serializers
    struct SerializerPair {
        std::function<std::vector<uint8_t>(const void *)> serializer;
        std::function<void(void *, const uint8_t *, size_t)> deserializer;
        ComponentID id;
    };

    std::map<size_t, SerializerPair> m_customSerializers; // keyed by typeid hash
};

// Template implementations

template <typename T>
SerializedComponent ComponentSerializer::Serialize(const T &component,
                                                   ComponentID id) {
    SerializedComponent result;
    result.id = id;
    result.data.resize(sizeof(T));
    std::memcpy(result.data.data(), &component, sizeof(T));
    return result;
}

template <typename T>
T ComponentSerializer::Deserialize(const uint8_t *data, size_t size) {
    T component;
    if (size >= sizeof(T)) {
        std::memcpy(&component, data, sizeof(T));
    }
    return component;
}

template <typename T>
void ComponentSerializer::RegisterSerializer(
    ComponentID id, std::function<std::vector<uint8_t>(const T &)> serializer,
    std::function<T(const uint8_t *, size_t)> deserializer) {
    size_t typeHash = typeid(T).hash_code();

    SerializerPair pair;
    pair.id = id;
    pair.serializer = [serializer](const void *obj) -> std::vector<uint8_t> {
        return serializer(*static_cast<const T *>(obj));
    };
    pair.deserializer = [deserializer](void *obj, const uint8_t *data,
                                       size_t size) {
        *static_cast<T *>(obj) = deserializer(data, size);
    };

    m_customSerializers[typeHash] = pair;
}

template <typename T>
bool ComponentSerializer::HasCustomSerializer() const {
    size_t typeHash = typeid(T).hash_code();
    return m_customSerializers.find(typeHash) != m_customSerializers.end();
}

template <typename T>
SerializedComponent ComponentSerializer::SerializeCustom(const T &component) {
    size_t typeHash = typeid(T).hash_code();
    auto it = m_customSerializers.find(typeHash);
    if (it != m_customSerializers.end()) {
        SerializedComponent result;
        result.id = it->second.id;
        result.data = it->second.serializer(&component);
        return result;
    }
    // Fallback to default
    return Serialize(component, ComponentID::Transform);
}

template <typename T>
T ComponentSerializer::DeserializeCustom(const uint8_t *data, size_t size) {
    size_t typeHash = typeid(T).hash_code();
    auto it = m_customSerializers.find(typeHash);
    if (it != m_customSerializers.end()) {
        T component;
        it->second.deserializer(&component, data, size);
        return component;
    }
    // Fallback to default
    return Deserialize<T>(data, size);
}
