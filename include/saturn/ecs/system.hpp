#ifndef SATURN_SYSTEM_HPP
#define SATURN_SYSTEM_HPP

#include "ecs_core.hpp"
#include "query.hpp"
#include "trait_helpers.h"

namespace saturn {

class system_context {
    friend class world;

    _::ecs_core* _core;
    delta_time _dt;

    system_context(_::ecs_core* core, delta_time dt) : _core(core), _dt(dt) { }

  public:
    [[nodiscard]] delta_time dt() const {
        return _dt;
    }
};

template <typename... T>
using system_query_results = query<T...>;
template <typename... T>
using system_func = std::function<void(system_query_results<T...>& query)>;
template <typename... T>
using system_func_with_context = std::function<void(system_context& ctx, system_query_results<T...>& query)>;

template <typename... T>
struct system {
    using query_t = system_query_results<T...>;
    using ctx_t = saturn::system_context;

    template <template <typename...> typename F>
    using forward_args_t = F<T...>;

    virtual ~system() = default;
    virtual void run(ctx_t& ctx, query_t& query) = 0;
};

namespace _ {

class system_base {
  public:
    virtual ~system_base() = default;
    virtual void run(system_context& ctx) = 0;
};

template <typename... T>
class func_system : public system_base {
    friend class world;

    query<T...> _query;
    system_func<T...> _func;

  public:
    func_system(query<T...> query, system_func<T...> callback) : _query(query), _func(callback) { }

    void run(system_context& ctx) override {
        _func(_query);
    }
};

template <typename... T>
class func_system_with_ctx : public system_base {
    friend class world;

    query<T...> _query;
    system_func_with_context<T...> _func;

  public:
    func_system_with_ctx(query<T...> query, system_func_with_context<T...> callback)
        : _query(query), _func(callback) { }

    void run(system_context& ctx) override {
        _func(ctx, _query);
    }
};

template <typename... T>
class struct_ptr_system : public system_base {
    friend class world;

    query<T...> _query;
    std::unique_ptr<system<T...>> _system;

  public:
    struct_ptr_system(query<T...> query, std::unique_ptr<system<T...>> system)
        : _query(query), _system(std::move(system)) { }

    void run(system_context& ctx) override {
        _system->run(ctx, _query);
    }
};

} // namespace _

} // namespace saturn

#endif
