#include "document/ChannelManager.h"

#include <algorithm>

namespace snapper {

ChannelManager::ChannelManager(QObject* parent) : QObject(parent) {}

quint64 ChannelManager::KeyOf(Id target, ChannelProperty property) {
  return (static_cast<quint64>(static_cast<quint32>(target)) << 8U) |
         static_cast<quint64>(property);
}

bool ChannelManager::Put(Channel channel) {
  if (channel.target == kNoId) {
    return false;
  }
  std::ranges::stable_sort(channel.keys, {}, &Key::frame);
  std::erase_if(channel.keys, [](const Key& key) { return key.frame < 1; });
  const auto repeats = std::ranges::unique(channel.keys, {}, &Key::frame);
  channel.keys.erase(repeats.begin(), repeats.end());
  if (static_cast<int>(channel.keys.size()) > kMaxKeysPerChannel) {
    return false;
  }

  const quint64 key = KeyOf(channel.target, channel.property);
  const auto found = index_.constFind(key);
  const bool exists = found != index_.cend();
  const Id target = channel.target;
  if (channel.keys.empty()) {
    if (exists) {
      channels_.erase(channels_.begin() + *found);
      RebuildIndex();
      Q_EMIT ChannelChanged(target);
    }
    return true;
  }
  if (exists) {
    channels_[static_cast<size_t>(*found)] = std::move(channel);
  } else {
    if (Count() >= kMaxChannels) {
      return false;
    }
    index_.insert(key, Count());
    channels_.push_back(std::move(channel));
  }
  Q_EMIT ChannelChanged(target);
  return true;
}

void ChannelManager::Clear() {
  channels_.clear();
  index_.clear();
  Q_EMIT ChannelChanged(kNoId);
}

const Channel* ChannelManager::Find(Id target, ChannelProperty property) const {
  const auto found = index_.constFind(KeyOf(target, property));
  return found == index_.cend() ? nullptr
                                : &channels_[static_cast<size_t>(*found)];
}

Channel ChannelManager::Get(Id target, ChannelProperty property) const {
  const Channel* found = Find(target, property);
  return found != nullptr
             ? *found
             : Channel{.target = target, .property = property, .keys = {}};
}

void ChannelManager::RebuildIndex() {
  index_.clear();
  for (size_t i = 0; i < channels_.size(); ++i) {
    index_.insert(KeyOf(channels_[i].target, channels_[i].property),
                  static_cast<int>(i));
  }
}

}  // namespace snapper
