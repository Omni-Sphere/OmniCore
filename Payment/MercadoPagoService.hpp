#pragma once
#include "Payment/Repositories/MercadoPagoRepository.hpp"
#include "Payment/Models/PayableEntity.hpp"
#include "Authorization/Models/SecurityContext.hpp"
#include <OmniData/DatabasePool.hpp>
#include <memory>
#include <string>
#include <optional>

namespace omnisphere::services
{
    struct MercadoPagoPaymentResult
    {
        bool success = false;
        std::string paymentId;
        std::string status;       // approved, pending, in_process, rejected, etc.
        std::string statusDetail; // accredited, cc_rejected_insufficient_amount, etc.
        std::string ticketUrl;
        std::string clabe;
        std::string errorMessage;
    };

    struct MercadoPagoPreferenceResult
    {
        bool success = false;
        std::string checkoutUrl;
        std::string preferenceId;
        std::string errorMessage;
    };

    struct MercadoPagoDiagnosticResult
    {
        bool isConfigured = false;
        bool isFunctional = false;
        std::string message;
    };

    class MercadoPagoService
    {
    public:
        explicit MercadoPagoService(std::shared_ptr<omnisphere::data::DatabasePool> dbPool);
        explicit MercadoPagoService(std::shared_ptr<omnisphere::repositories::MercadoPagoRepository> repository);

        std::optional<omnisphere::models::MercadoPagoSettings> GetSettings(bool decryptKeys = false) const;
        bool SaveSettings(const omnisphere::models::MercadoPagoSettings& settings) const;

        // Checkout API (Cobro directo con token generado por front en formulario propio)
        MercadoPagoPaymentResult CreatePayment(
            const omnisphere::models::SecurityContext& ctx,
            const omnisphere::models::PayableEntity& entity,
            const std::string& token,
            const std::string& paymentMethodId = "visa",
            const std::string& issuerId = "",
            int installments = 1
        ) const;

        // Checkout Pro (Preferencia de Mercado Pago con init_point / modal)
        MercadoPagoPreferenceResult CreatePreference(
            const omnisphere::models::SecurityContext& ctx,
            const omnisphere::models::PayableEntity& entity
        ) const;

        // Transferencia bancaria (SPEI / OXXO vía Checkout API)
        MercadoPagoPaymentResult CreateBankTransfer(
            const omnisphere::models::SecurityContext& ctx,
            const omnisphere::models::PayableEntity& entity,
            const std::string& paymentMethodId = "spei"
        ) const;

        MercadoPagoDiagnosticResult TestIntegration(const omnisphere::models::SecurityContext& ctx) const;

        // Consultar estado de un pago directamente en Mercado Pago
        std::optional<boost::json::object> GetPaymentFromMP(const std::string& paymentId) const;

    private:
        std::shared_ptr<omnisphere::data::DatabasePool> m_dbPool;
        std::shared_ptr<omnisphere::repositories::MercadoPagoRepository> m_repository;

        // Helper genérico para peticiones HTTPS a api.mercadopago.com usando Boost.Beast
        std::string ExecuteHttpsRequest(
            const std::string& method,
            const std::string& target,
            const std::string& accessToken,
            const std::string& jsonBody = "",
            const std::string& idempotencyKey = ""
        ) const;
    };
} // namespace omnisphere::services
