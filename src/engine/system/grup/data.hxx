#pragma once
#include <shared_mutex>

namespace engine::system::grup {
  struct MetaTexturesKey {
    std::string path;
    VkFormat format;

    bool operator==(const MetaTexturesKey& other) const {
      return (true
        && path == other.path
        && format == other.format
      );
    }
  };
}

namespace std {
  template <>
  struct hash<engine::system::grup::MetaTexturesKey> {
    size_t operator()(const engine::system::grup::MetaTexturesKey& key) const {
      return hash<std::string>()(key.path) ^ hash<VkFormat>()(key.format);
    }
  };
}

namespace engine::system::grup {

struct Groups {
  struct Item {
    lib::Lifetime lifetime;
    std::string name;
  };

  std::unordered_map<lib::GUID, Item> items;
  std::shared_mutex rw_mutex;
};

struct MetaMeshes {
  std::unordered_map<std::string, lib::GUID> by_path;

  enum struct Status {
    Loading,
    Ready,
  };

  struct Item {
    size_t ref_count;
    Status status;
    bool invalid;
    lib::Task *will_have_loaded;
    lib::Task *will_have_reloaded;
    std::string path;
  } item;

  std::unordered_map<lib::GUID, Item> items;
};

struct MetaTextures {
  std::unordered_map<MetaTexturesKey, lib::GUID> by_key;

  enum struct Status {
    Loading,
    Ready,
  };

  struct Item {
    size_t ref_count;
    Status status;
    bool invalid;
    lib::Task *will_have_loaded;
    lib::Task *will_have_reloaded;
    std::string path;
    VkFormat format;
  } item;

  std::unordered_map<lib::GUID, Item> items;
};

} // namespace
