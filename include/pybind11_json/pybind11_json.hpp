/***************************************************************************
* Copyright (c) 2019, Martin Renou                                         *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#ifndef PYBIND11_JSON_HPP
#define PYBIND11_JSON_HPP

#include <string>
#include <vector>

#include "nlohmann/json.hpp"

#include "pybind11/pybind11.h"

namespace py = pybind11;
namespace nl = nlohmann;

namespace nlohmann
{

    namespace detail
    {
        inline py::object from_json_impl(const json& j)
        {
            if (j.is_null())
            {
                return py::none();
            }
            else if (j.is_boolean())
            {
                return py::bool_(j.get<bool>());
            }
            else if (j.is_number())
            {
                double number = j.get<double>();
                if (number == std::floor(number))
                {
                    return py::int_(j.get<long>());
                }
                else
                {
                    return py::float_(number);
                }
            }
            else if (j.is_string())
            {
                return py::str(j.get<std::string>());
            }
            else if (j.is_array())
            {
                py::list obj;
                for (const auto& el : j)
                {
                    obj.attr("append")(from_json_impl(el));
                }
                return obj;
            }
            else // Object
            {
                py::dict obj;
                for (json::const_iterator it = j.cbegin(); it != j.cend(); ++it)
                {
                    obj[py::str(it.key())] = from_json_impl(it.value());
                }
                return obj;
            }
        }

        inline json to_json_impl(const py::handle& obj)
        {
            if (obj.ptr() == nullptr || obj.is_none())
            {
                return nullptr;
            }
            if (py::isinstance<py::bool_>(obj))
            {
                return obj.cast<bool>();
            }
            if (py::isinstance<py::int_>(obj))
            {
                return obj.cast<long>();
            }
            if (py::isinstance<py::float_>(obj))
            {
                return obj.cast<double>();
            }
            if (py::isinstance<py::str>(obj))
            {
                return obj.cast<std::string>();
            }
            if (py::isinstance<py::tuple>(obj) || py::isinstance<py::list>(obj))
            {
                auto out = json::array();
                for (const py::handle& value : obj)
                {
                    out.push_back(to_json_impl(value));
                }
                return out;
            }
            if (py::isinstance<py::dict>(obj))
            {
                auto out = json::object();
                for (const py::handle& key : obj)
                {
                    out[py::str(key).cast<std::string>()] = to_json_impl(obj[key]);
                }
                return out;
            }
            throw std::runtime_error("to_json not implemented for this type of object: " + py::repr(obj).cast<std::string>());
        }
    }

    #define PYBIND11_JSON_MAKE_SERIALIZER(T)               \
    template <>                                            \
    struct adl_serializer<T>                               \
    {                                                      \
        inline static T from_json(const json& j)           \
        {                                                  \
            return detail::from_json_impl(j);              \
        }                                                  \
                                                           \
        inline static void to_json(json& j, const T& obj)  \
        {                                                  \
            j = detail::to_json_impl(obj);                 \
        }                                                  \
    };

    PYBIND11_JSON_MAKE_SERIALIZER(py::handle);
    PYBIND11_JSON_MAKE_SERIALIZER(py::object);

    PYBIND11_JSON_MAKE_SERIALIZER(py::bool_);
    PYBIND11_JSON_MAKE_SERIALIZER(py::int_);
    PYBIND11_JSON_MAKE_SERIALIZER(py::float_);
    PYBIND11_JSON_MAKE_SERIALIZER(py::str);

    PYBIND11_JSON_MAKE_SERIALIZER(py::list);
    PYBIND11_JSON_MAKE_SERIALIZER(py::tuple);
    PYBIND11_JSON_MAKE_SERIALIZER(py::dict);

    #undef PYBIND11_JSON_MAKE_SERIALIZER

}

namespace pybind11
{
    namespace detail
    {
        template <> struct type_caster<nl::json>
        {
        public:
            PYBIND11_TYPE_CASTER(nl::json, _("json"));

            bool load(handle src, bool)
            {
                try {
                    value = nl::detail::to_json_impl(src);
                    return true;
                }
                catch(...)
                {
                    return false;
                }
            }

            static handle cast(nl::json src, return_value_policy /* policy */, handle /* parent */)
            {
                object obj = nl::detail::from_json_impl(src);
                return obj.release();
            }
        };
    }
}

#endif
