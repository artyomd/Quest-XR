//
// Created by artyomd on 2/2/21.
//

#pragma once

template<typename... Ts>
struct make_overload : Ts ... { using Ts::operator()...; };
template<typename... Ts> make_overload(Ts...) -> make_overload<Ts...>;

template<typename Variant, typename... Alternatives>
decltype(auto) VisitVariant(Variant &&variant, Alternatives &&... alternatives) {
  return std::visit(
      make_overload{std::forward<Alternatives>(alternatives)...},
      std::forward<Variant>(variant)
  );
}
