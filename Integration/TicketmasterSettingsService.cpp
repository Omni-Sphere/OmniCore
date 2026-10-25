#include "Integration/TicketmasterSettingsService.hpp"
#include <OmniData/DataMapper.hpp>
#include <OmniUtils/Base64.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace omnisphere::services
{
    TicketmasterSettingsService::TicketmasterSettingsService(std::shared_ptr<omnisphere::data::DatabasePool> dbPool)
        : m_repository(std::make_shared<omnisphere::repositories::TicketmasterRepository>(std::move(dbPool))) {}

    TicketmasterSettingsService::TicketmasterSettingsService(std::shared_ptr<omnisphere::repositories::TicketmasterRepository> repository)
        : m_repository(std::move(repository)) {}

    std::optional<omnisphere::models::TicketmasterSettings> TicketmasterSettingsService::GetSettings(bool decryptKeys) const
    {
        if (!m_repository) return std::nullopt;
        auto dt = m_repository->GetSettings();
        if (dt.IsEmpty())
        {
            omnisphere::models::TicketmasterSettings defaultSettings;
            return defaultSettings;
        }

        auto s = omnisphere::types::FromDataRow<omnisphere::models::TicketmasterSettings>(dt[0]);
        if (decryptKeys)
        {
            // Solo para peticiones internas en memoria a la API de Ticketmaster
            auto decryptVal = [](const std::string& val) -> std::string {
                if (val.empty()) return "";
                try {
                    return omnisphere::utils::Base64::Decode(val);
                } catch (...) {
                    return val;
                }
            };

            s.apiKey = decryptVal(s.apiKey);
            s.apiSecret = decryptVal(s.apiSecret);
        }
        // Si decryptKeys == false (para GraphQL / Frontend), se mantienen siempre codificadas en Base64
        return s;
    }

    bool TicketmasterSettingsService::SaveSettings(const omnisphere::models::TicketmasterSettings& settings, const std::vector<std::string>& mutationFields) const
    {
        if (!m_repository) return false;

        omnisphere::models::TicketmasterSettings toSave = settings;
        auto existingDt = m_repository->GetSettings({"Entry", "Code", "ApiKey", "ApiSecret"});
        omnisphere::models::TicketmasterSettings existingSettings{};
        if (!existingDt.IsEmpty())
        {
            existingSettings = omnisphere::types::FromDataRow<omnisphere::models::TicketmasterSettings>(existingDt[0]);
        }

        auto encodeIfNeeded = [](const std::string& input, const std::string& existingVal) -> std::string {
            if (input.empty()) return "";
            if (!existingVal.empty() && input == existingVal) return existingVal;
            return omnisphere::utils::Base64::Encode(input);
        };

        toSave.apiKey = encodeIfNeeded(toSave.apiKey, existingSettings.apiKey);
        toSave.apiSecret = encodeIfNeeded(toSave.apiSecret, existingSettings.apiSecret);

        if (existingDt.IsEmpty())
        {
            if (toSave.code.empty()) toSave.code = "DEFAULT";
            return m_repository->Create(toSave, mutationFields);
        }
        else
        {
            toSave.code = existingSettings.code.empty() ? "DEFAULT" : existingSettings.code;
            return m_repository->Update(toSave, mutationFields);
        }
    }

    TicketmasterTestIntegrationResult TicketmasterSettingsService::TestIntegration() const
    {
        TicketmasterTestIntegrationResult res;
        auto settingsOpt = GetSettings(true); // Decodifica la llave exclusivamente en memoria

        if (!settingsOpt.has_value() || !settingsOpt->isActive)
        {
            res.isConfigured = false;
            res.isFunctional = false;
            res.message = "La integración de Ticketmaster no está configurada o se encuentra desactivada.";
            return res;
        }

        auto settings = settingsOpt.value();
        if (settings.apiKey.empty())
        {
            res.isConfigured = false;
            res.isFunctional = false;
            res.message = "Configuración incompleta: Falta la API Key (Consumer Key).";
            return res;
        }

        res.isConfigured = true;

        try
        {
            // Extraer host de la URL configurada (ej. https://app.ticketmaster.com)
            std::string host = "app.ticketmaster.com";
            std::string baseUrl = settings.apiBaseUrl;
            auto schemePos = baseUrl.find("://");
            if (schemePos != std::string::npos)
            {
                baseUrl = baseUrl.substr(schemePos + 3);
            }
            auto slashPos = baseUrl.find('/');
            if (slashPos != std::string::npos)
            {
                host = baseUrl.substr(0, slashPos);
            }
            else if (!baseUrl.empty())
            {
                host = baseUrl;
            }

            std::string port = "443";
            std::string version = settings.apiVersion.empty() ? "v2" : settings.apiVersion;
            std::string country = settings.defaultCountryCode.empty() ? "MX" : settings.defaultCountryCode;
            std::string target = "/discovery/" + version + "/events.json?apikey=" + settings.apiKey + "&countryCode=" + country + "&size=1";

            boost::asio::io_context ioc;
            boost::asio::ssl::context sslCtx(boost::asio::ssl::context::tlsv12_client);
            sslCtx.set_default_verify_paths();

            boost::asio::ip::tcp::resolver resolver(ioc);
            boost::asio::ssl::stream<boost::asio::ip::tcp::socket> stream(ioc, sslCtx);

            if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
            {
                res.isFunctional = false;
                res.message = "Error configurando SNI SSL para Ticketmaster.";
                return res;
            }

            auto const results = resolver.resolve(host, port);
            boost::asio::connect(stream.next_layer(), results.begin(), results.end());
            stream.handshake(boost::asio::ssl::stream_base::client);

            boost::beast::http::request<boost::beast::http::empty_body> req{boost::beast::http::verb::get, target, 11};
            req.set(boost::beast::http::field::host, host);
            req.set(boost::beast::http::field::user_agent, "OmniSphere-C++/1.0");
            req.set(boost::beast::http::field::accept, "application/json");
            req.prepare_payload();

            boost::beast::http::write(stream, req);

            boost::beast::flat_buffer buffer;
            boost::beast::http::response<boost::beast::http::dynamic_body> response;
            boost::beast::http::read(stream, buffer, response);

            int status = static_cast<int>(response.result_int());
            if (status == 200)
            {
                res.isFunctional = true;
                std::string body = boost::beast::buffers_to_string(response.body().data());
                std::string extraMsg = "";
                auto totalPos = body.find("\"totalElements\":");
                if (totalPos != std::string::npos)
                {
                    auto commaPos = body.find_first_of(",}", totalPos);
                    if (commaPos != std::string::npos)
                    {
                        std::string totalStr = body.substr(totalPos + 16, commaPos - (totalPos + 16));
                        extraMsg = " Se detectaron " + totalStr + " eventos disponibles en " + country + ".";
                    }
                }
                res.message = "¡Conexión exitosa con Ticketmaster Discovery API! Credenciales válidas." + extraMsg;
            }
            else if (status == 401 || status == 403)
            {
                res.isFunctional = false;
                res.message = "Error de autenticación Ticketmaster: Consumer Key no autorizada o inválida (HTTP " + std::to_string(status) + ").";
            }
            else if (status == 429)
            {
                res.isFunctional = false;
                res.message = "Límite de peticiones de Ticketmaster excedido (Rate limit 429).";
            }
            else
            {
                res.isFunctional = false;
                res.message = "Ticketmaster respondió con código HTTP " + std::to_string(status) + ".";
            }
        }
        catch (const std::exception& ex)
        {
            res.isFunctional = false;
            res.message = std::string("Excepción al conectar con Ticketmaster: ") + ex.what();
        }

        return res;
    }

    static std::string NormalizeString(const std::string& input)
    {
        std::string out;
        for (char c : input)
        {
            if (std::isalnum(static_cast<unsigned char>(c)))
            {
                out += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
        }
        return out;
    }

    static std::string UrlEncode(const std::string& value)
    {
        std::ostringstream escaped;
        escaped.fill('0');
        escaped << std::hex;
        for (char c : value)
        {
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~')
            {
                escaped << c;
            }
            else if (c == ' ')
            {
                escaped << "%20";
            }
            else
            {
                escaped << '%' << std::setw(2) << ((int)static_cast<unsigned char>(c));
            }
        }
        return escaped.str();
    }

    std::vector<TicketmasterVenue> TicketmasterSettingsService::PreviewVenues(
        const std::string& city,
        const std::string& countryCode,
        int size) const
    {
        std::vector<TicketmasterVenue> results;
        auto settingsOpt = GetSettings(true);
        if (!settingsOpt.has_value() || settingsOpt->apiKey.empty())
        {
            return results;
        }

        auto settings = settingsOpt.value();
        std::string host = "app.ticketmaster.com";
        std::string baseUrl = settings.apiBaseUrl;
        auto schemePos = baseUrl.find("://");
        if (schemePos != std::string::npos) baseUrl = baseUrl.substr(schemePos + 3);
        auto slashPos = baseUrl.find('/');
        if (slashPos != std::string::npos) host = baseUrl.substr(0, slashPos);
        else if (!baseUrl.empty()) host = baseUrl;

        std::string version = settings.apiVersion.empty() ? "v2" : settings.apiVersion;
        std::string targetCity = !city.empty() ? city : settings.defaultCity.value_or("Ciudad de México");
        std::string targetCountry = !countryCode.empty() ? countryCode : (!settings.defaultCountryCode.empty() ? settings.defaultCountryCode : "MX");
        int targetSize = (size <= 0 || size > 50) ? 20 : size;

        std::string target = "/discovery/" + version + "/venues.json?apikey=" + settings.apiKey +
                             "&countryCode=" + targetCountry +
                             "&city=" + UrlEncode(targetCity) +
                             "&size=" + std::to_string(targetSize) +
                             "&sort=relevance,desc";

        std::cout << "\n=======================================================\n";
        std::cout << "[Ticketmaster::PreviewVenues] INICIANDO PETICION EXTERNA\n";
        std::cout << "Host: " << host << ":443\n";
        std::cout << "Target: /discovery/" << version << "/venues.json?apikey=***&countryCode=" << targetCountry 
                  << "&city=" << targetCity << "&size=" << targetSize << "\n";
        std::cout << "=======================================================\n" << std::endl;

        std::string responseBody;
        int status = 0;
        try
        {
            boost::asio::io_context ioc;
            boost::asio::ssl::context sslCtx(boost::asio::ssl::context::tlsv12_client);
            sslCtx.set_default_verify_paths();

            boost::asio::ip::tcp::resolver resolver(ioc);
            boost::asio::ssl::stream<boost::asio::ip::tcp::socket> stream(ioc, sslCtx);

            if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
            {
                std::cerr << "[Ticketmaster::PreviewVenues] Error SSL_set_tlsext_host_name para " << host << std::endl;
                return results;
            }

            auto const endpoints = resolver.resolve(host, "443");
            boost::asio::connect(stream.next_layer(), endpoints.begin(), endpoints.end());
            stream.handshake(boost::asio::ssl::stream_base::client);

            boost::beast::http::request<boost::beast::http::empty_body> req{boost::beast::http::verb::get, target, 11};
            req.set(boost::beast::http::field::host, host);
            req.set(boost::beast::http::field::user_agent, "OmniSphere-C++/1.0");
            req.set(boost::beast::http::field::accept, "application/json");
            req.prepare_payload();

            boost::beast::http::write(stream, req);

            boost::beast::flat_buffer buffer;
            boost::beast::http::response<boost::beast::http::dynamic_body> response;
            boost::beast::http::read(stream, buffer, response);

            status = static_cast<int>(response.result_int());
            responseBody = boost::beast::buffers_to_string(response.body().data());

            std::cout << "\n=======================================================\n";
            std::cout << "[Ticketmaster::PreviewVenues] RESPUESTA RECIBIDA\n";
            std::cout << "Status HTTP: " << status << "\n";
            std::cout << "Body Preview (primeros 500 chars):\n" 
                      << responseBody.substr(0, 500) << "\n";
            std::cout << "=======================================================\n" << std::endl;
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[Ticketmaster::PreviewVenues EXCEPCION HTTP] " << ex.what() << std::endl;
            return results;
        }

        if (status != 200 || responseBody.empty())
        {
            std::cerr << "[Ticketmaster::PreviewVenues] Fallo en la llamada: HTTP Status " << status << std::endl;
            return results;
        }

        // Leer lugares ya registrados en nuestra base de datos para comparar / unir
        auto existingVenues = m_repository ? m_repository->GetExistingVenues() : std::vector<std::pair<std::string, std::string>>{};

        try
        {
            auto jsonVal = boost::json::parse(responseBody);
            if (!jsonVal.is_object())
            {
                std::cerr << "[Ticketmaster::PreviewVenues] El JSON raíz no es un objeto válido" << std::endl;
                return results;
            }
            auto const& obj = jsonVal.as_object();
            if (!obj.contains("_embedded") || !obj.at("_embedded").is_object())
            {
                std::cout << "[Ticketmaster::PreviewVenues] La respuesta no contiene '_embedded' (0 recintos encontrados)" << std::endl;
                return results;
            }

            auto const& embedded = obj.at("_embedded").as_object();
            if (!embedded.contains("venues") || !embedded.at("venues").is_array())
            {
                std::cout << "[Ticketmaster::PreviewVenues] No se encontró el arreglo 'venues' en '_embedded'" << std::endl;
                return results;
            }

            auto const& venuesArr = embedded.at("venues").as_array();
            std::cout << "[Ticketmaster::PreviewVenues] Recintos parseados: " << venuesArr.size() << std::endl;
            for (auto const& itemVal : venuesArr)
            {
                if (!itemVal.is_object()) continue;
                auto const& vObj = itemVal.as_object();

                TicketmasterVenue tv;
                if (vObj.contains("id") && vObj.at("id").is_string())
                    tv.id = std::string(vObj.at("id").as_string());
                if (vObj.contains("name") && vObj.at("name").is_string())
                    tv.name = std::string(vObj.at("name").as_string());
                if (vObj.contains("url") && vObj.at("url").is_string())
                    tv.url = std::string(vObj.at("url").as_string());
                if (vObj.contains("postalCode") && vObj.at("postalCode").is_string())
                    tv.postalCode = std::string(vObj.at("postalCode").as_string());

                if (vObj.contains("city") && vObj.at("city").is_object())
                {
                    auto const& cityObj = vObj.at("city").as_object();
                    if (cityObj.contains("name") && cityObj.at("name").is_string())
                    {
                        tv.city = std::string(cityObj.at("name").as_string());
                    }
                }
                if (tv.city.empty()) tv.city = targetCity;

                if (vObj.contains("address") && vObj.at("address").is_object())
                {
                    auto const& addrObj = vObj.at("address").as_object();
                    if (addrObj.contains("line1") && addrObj.at("line1").is_string())
                    {
                        tv.address = std::string(addrObj.at("line1").as_string());
                    }
                }

                // Comparar contra venues de nuestra base de datos (por similitud de nombre)
                std::string normTm = NormalizeString(tv.name);
                for (const auto& [dbName, dbCode] : existingVenues)
                {
                    std::string normDb = NormalizeString(dbName);
                    if (normTm == normDb ||
                        (!normTm.empty() && normDb.find(normTm) != std::string::npos) ||
                        (!normDb.empty() && normTm.find(normDb) != std::string::npos))
                    {
                        tv.existsInDatabase = true;
                        tv.existingVenueCode = dbCode;
                        break;
                    }
                }

                results.push_back(tv);
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "[TicketmasterSettingsService::PreviewVenues Parse Exception] " << ex.what() << std::endl;
        }

        return results;
    }
}
