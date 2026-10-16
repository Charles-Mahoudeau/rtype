/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** QueryBuilder
*/

#include "QueryBuilder.hpp"

#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/query/DynamicQuery.hpp"
#include "rtype/ecs/query/QueryTerm.hpp"

namespace rtype::ecs {
QueryBuilder& QueryBuilder::read(ComponentId id) { return add(id, TermKind::kRead); }

QueryBuilder& QueryBuilder::write(ComponentId id) { return add(id, TermKind::kWrite); }

QueryBuilder& QueryBuilder::optionalRead(ComponentId id) { return add(id, TermKind::kOptionalRead); }

QueryBuilder& QueryBuilder::optionalWrite(ComponentId id) { return add(id, TermKind::kOptionalWrite); }

QueryBuilder& QueryBuilder::with(ComponentId id) { return add(id, TermKind::kWith); }

QueryBuilder& QueryBuilder::without(ComponentId id) { return add(id, TermKind::kWithout); }

QueryBuilder& QueryBuilder::added(ComponentId id) { return add(id, TermKind::kAdded); }

QueryBuilder& QueryBuilder::changed(ComponentId id) { return add(id, TermKind::kChanged); }

Result<DynamicQuery> QueryBuilder::build(Components& components) const {
    return DynamicQuery::create(components, _terms);
}

QueryBuilder& QueryBuilder::add(ComponentId id, TermKind kind) {
    _terms.push_back(QueryTerm{.id = id, .kind = kind});
    return *this;
}
}  // namespace rtype::ecs
