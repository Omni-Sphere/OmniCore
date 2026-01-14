#include "Repositories/GlobalConfiguration.hpp"
#include <stdexcept>
#include <vector>
#include <iostream>

namespace omnicore::repository
{
    GlobalConfiguration::GlobalConfiguration(std::shared_ptr<service::Database> _database) : database(std::move(_database)) {}

    bool GlobalConfiguration::Update(const dto::UpdateGlobalConfiguration &config) const
    {
        try
        {
            std::string sQuery = "UPDATE GlobalConfiguration SET ";
            std::vector<type::SQLParam> updateParams;

            if (config.ImagePath.has_value())
            {
                sQuery += "ImagePath = ?";
                updateParams.emplace_back(type::MakeSQLParam(config.ImagePath.value()));
            }

            if (config.PDFPath.has_value())
            {
                sQuery += ", PDFPath = ?";
                updateParams.emplace_back(type::MakeSQLParam(config.PDFPath.value()));
            }

            if (config.XMLPath.has_value())
            {
                sQuery += ", XMLPath = ?";
                updateParams.emplace_back(type::MakeSQLParam(config.XMLPath.value()));
            }

            if (config.PasswordExpirationDays.has_value())
            {
                sQuery += ", PasswordExpirationDays = ?";
                updateParams.emplace_back(type::MakeSQLParam(config.PasswordExpirationDays.value()));
            }

            
            database->BeginTransaction();

            if (!database->RunPrepared(sQuery, updateParams))
            {
                database->RollbackTransaction();
                return false;
            }

            database->CommitTransaction();
            return true;
        }
        catch (const std::exception &e)
        {
            database->RollbackTransaction();
            throw std::runtime_error(std::string("[GlobalConfiguration Update Exception] ") + e.what());
        }
    }

    model::GlobalConfiguration GlobalConfiguration::Get(int confEntry) const
    {
        try
        {
            std::string sQuery = "SELECT ConfEntry, ImagePath, PDFPath, XMLPath, PasswordExpirationDays FROM GlobalConfiguration WHERE ConfEntry = ?";
            
            type::DataTable data = database->FetchPrepared(sQuery, std::to_string(confEntry));

            if (data.RowsCount() == 0)
            {
                throw std::runtime_error("Configuration not found");
            }

            model::GlobalConfiguration config;
            config.ConfEntry = data[0]["ConfEntry"];
            
            if (!data[0]["ImagePath"].IsNull())
                config.ImagePath = (std::string)data[0]["ImagePath"];
            
            if (!data[0]["PDFPath"].IsNull())
                config.PDFPath = (std::string)data[0]["PDFPath"];
                
            if (!data[0]["XMLPath"].IsNull())
                config.XMLPath = (std::string)data[0]["XMLPath"];
                
            config.PasswordExpirationDays = data[0]["PasswordExpirationDays"];

            return config;
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error(std::string("[GlobalConfiguration Get Exception] ") + e.what());
        }
    }
}
