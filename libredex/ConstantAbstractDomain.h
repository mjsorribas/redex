/**
 * Copyright (c) 2016-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#pragma once

#include <sstream>
#include <type_traits>
#include <utility>

#include <boost/optional.hpp>

#include "AbstractDomain.h"
#include "Debug.h"
#include "Show.h"

/*
 * This abstract domain combinator constructs the lattice of constants of a
 * certain type (also called the flat lattice or the three-level lattice). For
 * more detail on constant propagation please see:
 *
 *   https://www.cs.utexas.edu/users/lin/cs380c/wegman.pdf
 *
 * For example, the lattice of integer constants:
 *
 *                       TOP
 *                     /  |  \
 *           ... -2  -1   0   1  2 ....
 *                    \   |   /
 *                       _|_
 *
 * can be implemented as follows:
 *
 *   using Int32ConstantDomain = ConstantAbstractDomain<int32_t>;
 *
 * Note: The base constant elements should be comparable using `operator==()`.
 */

namespace acd_impl {

template <typename Constant>
class ConstantAbstractValue final
    : public AbstractValue<ConstantAbstractValue<Constant>> {
 public:
  using Kind = typename AbstractValue<ConstantAbstractValue>::Kind;

  ~ConstantAbstractValue() {
    static_assert(std::is_default_constructible<Constant>::value,
                  "Constant is not default constructible");
    static_assert(std::is_copy_constructible<Constant>::value,
                  "Constant is not copy constructible");
    static_assert(std::is_copy_assignable<Constant>::value,
                  "Constant is not copy assignable");
  }

  ConstantAbstractValue() = default;

  explicit ConstantAbstractValue(const Constant& constant)
      : m_constant(constant) {}

  void clear() override {}

  Kind kind() const override { return Kind::Value; }

  bool leq(const ConstantAbstractValue& other) const override {
    return equals(other);
  }

  bool equals(const ConstantAbstractValue& other) const override {
    return m_constant == other.get_constant();
  }

  Kind join_with(const ConstantAbstractValue& other) override {
    if (equals(other)) {
      return Kind::Value;
    }
    return Kind::Top;
  }

  Kind widen_with(const ConstantAbstractValue& other) override {
    return join_with(other);
  }

  Kind meet_with(const ConstantAbstractValue& other) override {
    if (equals(other)) {
      return Kind::Value;
    }
    return Kind::Bottom;
  }

  Kind narrow_with(const ConstantAbstractValue& other) override {
    return meet_with(other);
  }

  const Constant& get_constant() const { return m_constant; }

 private:
  Constant m_constant;
};

} // namespace acd_impl

template <typename Constant>
class ConstantAbstractDomain final
    : public AbstractDomainScaffolding<
          acd_impl::ConstantAbstractValue<Constant>,
          ConstantAbstractDomain<Constant>> {
 public:
  using AbstractValueKind =
      typename acd_impl::ConstantAbstractValue<Constant>::Kind;

  ConstantAbstractDomain() { this->set_to_top(); }

  explicit ConstantAbstractDomain(const Constant& cst) {
    this->set_to_value(acd_impl::ConstantAbstractValue<Constant>(cst));
  }

  explicit ConstantAbstractDomain(AbstractValueKind kind)
      : AbstractDomainScaffolding<acd_impl::ConstantAbstractValue<Constant>,
                                  ConstantAbstractDomain<Constant>>(kind) {}

  boost::optional<Constant> get_constant() const {
    return (this->kind() == AbstractValueKind::Value)
               ? boost::optional<Constant>(this->get_value()->get_constant())
               : boost::none;
  }

  static ConstantAbstractDomain bottom() {
    return ConstantAbstractDomain(AbstractValueKind::Bottom);
  }

  static ConstantAbstractDomain top() {
    return ConstantAbstractDomain(AbstractValueKind::Top);
  }

  std::string str() const;
};

template <typename Constant>
inline std::ostream& operator<<(std::ostream& out,
                                const ConstantAbstractDomain<Constant>& x) {
  using Kind = typename acd_impl::ConstantAbstractValue<Constant>::Kind;
  switch (x.kind()) {
  case Kind::Bottom: {
    out << "_|_";
    break;
  }
  case Kind::Top: {
    out << "T";
    break;
  }
  case Kind::Value: {
    out << *x.get_constant();
    break;
  }
  }
  return out;
}

template <typename Constant>
std::string ConstantAbstractDomain<Constant>::str() const {
  std::ostringstream os;
  os << *this;
  return os.str();
}
