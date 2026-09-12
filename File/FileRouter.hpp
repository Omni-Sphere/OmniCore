#pragma once
#include <OmniUtils/Http/Router.hpp>
#include <memory>
#include <string>

namespace omnisphere::services
{
    class FileRouter
    {
    public:
        static void RegisterEndpoints(
            std::shared_ptr<omnisphere::net::Router> router,
            const std::string& uploadDir = "./uploads"
        );
    };
}
