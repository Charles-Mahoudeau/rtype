/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** World
*/

namespace rtype::ecs {
template <Component T>
ComponentId World::component(ComponentOptions options) {
    const ComponentId id = _registry.registerComponent<T>(options);
    createPools(id);
    return id;
}
}  // namespace rtype::ecs
