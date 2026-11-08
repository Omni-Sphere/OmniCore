#pragma once
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <OmniData/DatabasePool.hpp>
#include "Integration/Models/TicketmasterSettings.hpp"
#include "Integration/Repositories/TicketmasterRepository.hpp"

namespace omnisphere::services
{
    struct TicketmasterTestIntegrationResult
    {
        bool isConfigured = false;
        bool isFunctional = false;
        std::string message;
    };

    struct TicketmasterVenue
    {
        std::string id;
        std::string name;
        std::string city;
        std::string address;
        std::string postalCode;
        std::string url;
        bool existsInDatabase = false;
        std::string existingVenueCode;
    };

    struct TicketmasterPageInfo
    {
        int totalElements = 0;
        int totalPages = 0;
        int pageNumber = 0;
        int pageSize = 20;
    };

    struct TicketmasterVenuePage
    {
        std::vector<TicketmasterVenue> items;
        TicketmasterPageInfo pageInfo;
    };

    struct TicketmasterEvent
    {
        std::string id;
        std::string title;
        std::string date;
        std::string imageUrl;
        std::string category;
        std::string venueName;
        std::string venueCity;
        std::string venueAddress;
        std::string url;
        bool existsInDatabase = false;
        std::string existingEventCode;
    };

    struct TicketmasterEventPage
    {
        std::vector<TicketmasterEvent> items;
        TicketmasterPageInfo pageInfo;
    };

    class TicketmasterSettingsService
    {
    private:
        std::shared_ptr<omnisphere::repositories::TicketmasterRepository> m_repository;

    public:
        explicit TicketmasterSettingsService(std::shared_ptr<omnisphere::data::DatabasePool> dbPool);
        explicit TicketmasterSettingsService(std::shared_ptr<omnisphere::repositories::TicketmasterRepository> repository);
        ~TicketmasterSettingsService() = default;

        std::optional<omnisphere::models::TicketmasterSettings> GetSettings(bool decryptKeys = false) const;
        bool SaveSettings(const omnisphere::models::TicketmasterSettings& settings, const std::vector<std::string>& mutationFields = {}) const;
        TicketmasterTestIntegrationResult TestIntegration() const;
        TicketmasterVenuePage PreviewVenues(const std::string& city = "Ciudad de México", const std::string& countryCode = "MX", const std::string& keyword = "", int page = 0, int size = 20) const;
        TicketmasterEventPage PreviewEvents(const std::string& city = "Ciudad de México", const std::string& countryCode = "MX", const std::string& classification = "Music", const std::string& keyword = "", int page = 0, int size = 20) const;
    };
}
