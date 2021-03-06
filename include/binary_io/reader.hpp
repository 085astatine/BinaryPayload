#ifndef BINARY_IO_READER_HPP_
#define BINARY_IO_READER_HPP_

#include <cassert>
#include <type_traits>
#include <boost/optional.hpp>
#include "binary_io/structure.hpp"

namespace binary_io {
template<typename Structure>
class Reader {
  using structure = Structure;
  using kind = typename structure::kind;

  template<kind key>
  using element = typename structure::template element<key>;

 public:
  // constructor
  Reader(const void* buffer_head,
         const std::size_t& buffer_size)
    : buffer_head_(buffer_head),
      buffer_size_(buffer_size) {
    assert(buffer_head_ || buffer_size_ == 0);
    assert(structure::byte_size() <= buffer_size_);
  }
  // getter: optional
  template<kind key, typename... Args>
  typename std::enable_if<
          impl::is_default_value_defined<element<key>>::value,
          boost::optional<typename element<key>::value_type>>::type
  GetOptional(Args&&... args) const {
    static_assert(key != kind::End, "End is reserved");
    static_assert(element<key>::key != kind::End, "invalid key");
    const auto bit_offset = structure::template bit_offset<key>();
    if (buffer_head_
        && (bit_offset + element<key>::bit_size <= buffer_size_ * 8)) {
      return element<key>::Read(
              buffer_head_,
              bit_offset,
              std::forward<Args>(args)...);
    }
    return boost::none;
  }
  // getter
  template<kind key, typename... Args>
  typename std::enable_if<
          impl::is_default_value_defined<element<key>>::value
          && !impl::is_structure<typename element<key>::value_type>::value,
          typename element<key>::value_type>::type
  Get(Args&&... args) const {
    if (const auto result = GetOptional<key>(std::forward<Args>(args)...)) {
      return *result;
    }
    return element<key>::DefaultValue();
  }
  // getter: DefaultValue is not defined
  template<kind key, typename... Args>
  typename std::enable_if<
          !impl::is_default_value_defined<element<key>>::value
          && !impl::is_structure<typename element<key>::value_type>::value,
          void>::type
  Get(Args&&... args) const {
    static_assert(key != kind::End, "End is reserved");
    static_assert(element<key>::key != kind::End, "invalid key");
    const auto bit_offset = structure::template bit_offset<key>();
    if (buffer_head_
        && (bit_offset + element<key>::bit_size <= buffer_size_ * 8)) {
      return element<key>::Read(
              buffer_head_,
              bit_offset,
              std::forward<Args>(args)...);
    }
  }
  // getter: strucutre
  template<kind key>
  typename std::enable_if<
          impl::is_structure<typename element<key>::value_type>::value,
          boost::optional<Reader<typename element<key>::value_type>>>::type
  Get() const {
    static_assert(key != kind::End, "End is reserved");
    static_assert(element<key>::key != kind::End, "invalid key");
    const auto bit_offset = structure::template bit_offset<key>();
    const auto byte_size = element<key>::value_type::byte_size();
    if (buffer_head_
        && bit_offset % 8 == 0
        && bit_offset / 8 + byte_size <= buffer_size_) {
      return Reader<typename element<key>::value_type>(
              reinterpret_cast<const uint8_t*>(buffer_head_) + bit_offset / 8,
              byte_size);
    } else {
      return boost::none;
    }
  }
  // head
  const void* Head() const {
    return buffer_head_;
  }

 private:
  const void* buffer_head_;
  std::size_t buffer_size_;
};
}  // namespace binary_io
#endif  // BINARY_IO_READER_HPP_
