-- =============================================================================
-- OmniCore.sql - Core Schema DDL (PostgreSQL Native Strict Preservation)
-- Modules: Identity, Users, Sessions, GlobalConfiguration, SystemConfig,
--          Multi-Gateway Payments (Stripe, OpenPay, MercadoPago), WhatsApp Cloud API
-- =============================================================================

-- 1. Custom ENUM Types Check
DO $$ 
BEGIN
    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'PermissionMode') THEN
        CREATE TYPE "PermissionMode" AS ENUM ('P', 'R');
    END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'PaymentMethodType') THEN
        CREATE TYPE "PaymentMethodType" AS ENUM (
            'NOT_APPLICABLE',
            'CASH',
            'TRANSFER',
            'CARD',
            'OTHER'
        );
    END IF;

    IF NOT EXISTS (SELECT 1 FROM pg_type WHERE typname = 'PaymentStatusType') THEN
        CREATE TYPE "PaymentStatusType" AS ENUM (
            'UNPAID',
            'PENDING',
            'PAID',
            'REFUNDED',
            'CANCELLED'
        );
    END IF;
END $$;

-- 2. Users
CREATE TABLE IF NOT EXISTS "Users" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(100) NOT NULL UNIQUE,
    "Email" VARCHAR(50),
    "Phone" VARCHAR(50),
    "Employee" INT,
    "Department" INT,
    "PermissionMode" "PermissionMode" NOT NULL DEFAULT 'P',
    "RoleEntry" INT,
    "MaxDisccountPerLine" NUMERIC(12, 6),
    "MaxDisccountPerDocument" NUMERIC(12, 6),
    "SuperUser" BOOLEAN NOT NULL DEFAULT false,
    "IsLocked" BOOLEAN NOT NULL DEFAULT false,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "Password" BYTEA,
    "ChangePasswordNextLogin" BOOLEAN NOT NULL DEFAULT false,
    "PasswordNeverExpires" BOOLEAN NOT NULL DEFAULT false,
    "CreatedBy" INT NOT NULL DEFAULT 0,
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" INT,
    "UpdateDate" TIMESTAMP
);

-- 3. Sessions
CREATE TABLE IF NOT EXISTS "Sessions" (
    "SessionEntry" SERIAL PRIMARY KEY,
    "SessionUUID" VARCHAR(100) NOT NULL UNIQUE DEFAULT gen_random_uuid()::text,
    "UserCode" VARCHAR(50),
    "UserEmail" VARCHAR(50),
    "UserPhone" VARCHAR(50),
    "StartDate" VARCHAR(50),
    "EndDate" VARCHAR(50),
    "DurationSeconds" INT DEFAULT 0,
    "DeviceIP" VARCHAR(50),
    "HostName" VARCHAR(100),
    "IsActive" CHAR(1) NOT NULL DEFAULT 'Y',
    "Reason" VARCHAR(255),
    "LogoutMessage" VARCHAR(255),
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE INDEX IF NOT EXISTS "IDX_Sessions_SessionUUID" ON "Sessions" ("SessionUUID");
CREATE INDEX IF NOT EXISTS "IDX_Sessions_UserCode_IsActive" ON "Sessions" ("UserCode", "IsActive");

-- 4. GlobalConfiguration
CREATE TABLE IF NOT EXISTS "GlobalConfiguration" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE DEFAULT 'DEFAULT',
    "Name" VARCHAR(100) NOT NULL,
    "Value" TEXT,
    "IsEncrypted" BOOLEAN NOT NULL DEFAULT false,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" INT NOT NULL DEFAULT 1,
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" INT,
    "UpdateDate" TIMESTAMP
);

-- 5. SystemConfigs
CREATE TABLE IF NOT EXISTS "SystemConfigs" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL DEFAULT 'DEFAULT' UNIQUE,
    "FeeHandlingStrategy" VARCHAR(50) NOT NULL DEFAULT 'SURCHARGE',
    "TaxRatePercent" NUMERIC(5, 2) NOT NULL DEFAULT 16.00,
    "DefaultCurrency" VARCHAR(10) NOT NULL DEFAULT 'MXN',
    "CompanyName" VARCHAR(255) NOT NULL DEFAULT 'OmniSphere Enterprise',
    "EnableEmailNotifications" BOOLEAN NOT NULL DEFAULT true,
    "EnableWhatsappNotifications" BOOLEAN NOT NULL DEFAULT true,
    "AllowPartialPayments" BOOLEAN NOT NULL DEFAULT false,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" INT NOT NULL DEFAULT 1,
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" INT,
    "UpdateDate" TIMESTAMP
);

-- 6. PaymentMethods
CREATE TABLE IF NOT EXISTS "PaymentMethods" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(255) NOT NULL,
    "Type" "PaymentMethodType" NOT NULL DEFAULT 'NOT_APPLICABLE',
    "UsesCommission" BOOLEAN NOT NULL DEFAULT false,
    "CommissionRate" NUMERIC(5, 2) NOT NULL DEFAULT 0.00,
    "UsesIntegration" BOOLEAN NOT NULL DEFAULT false,
    "IntegrationProvider" VARCHAR(50),
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" INT NOT NULL DEFAULT 0,
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" INT,
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_PaymentMethods_IsActive" CHECK ("IsActive" IN (true, false))
);

ALTER TABLE "PaymentMethods" ADD COLUMN IF NOT EXISTS "UsesIntegration" BOOLEAN NOT NULL DEFAULT false;
ALTER TABLE "PaymentMethods" ADD COLUMN IF NOT EXISTS "IntegrationProvider" VARCHAR(50);

-- 7. PaymentGateways (Configuración Dinámica Centralizada Multi-Pasarela)
CREATE TABLE IF NOT EXISTS "PaymentGateways" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Name" VARCHAR(255) NOT NULL,
    "Provider" VARCHAR(50) NOT NULL, -- 'STRIPE', 'OPENPAY', 'MERCADOPAGO', 'PAYPAL'
    "MerchantId" VARCHAR(255),
    "PublicKey" TEXT,
    "PrivateKey" TEXT,
    "WebhookSecret" TEXT,
    "ApiBaseUrl" VARCHAR(255),
    "Currency" VARCHAR(10) NOT NULL DEFAULT 'mxn',
    "IsTestMode" BOOLEAN NOT NULL DEFAULT true,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" INT NOT NULL DEFAULT 1,
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" INT,
    "UpdateDate" TIMESTAMP,
    CONSTRAINT "CHK_PaymentGateways_IsActive" CHECK ("IsActive" IN (true, false))
);

CREATE UNIQUE INDEX IF NOT EXISTS "UQ_PaymentGateways_Provider_Active" ON "PaymentGateways" ("Provider") WHERE "IsActive" = true;

INSERT INTO "PaymentGateways" ("Code", "Name", "Provider", "IsTestMode", "IsActive", "CreatedBy") VALUES
('GW_STRIPE', 'Stripe Gateway', 'STRIPE', true, true, 1),
('GW_OPENPAY', 'OpenPay México', 'OPENPAY', true, false, 1),
('GW_MERCADOPAGO', 'Mercado Pago Checkout Pro', 'MERCADOPAGO', true, false, 1)
ON CONFLICT ("Code") DO NOTHING;

-- 8. PaymentTransactions (Auditoría Universal de Transacciones de Cobro)
CREATE TABLE IF NOT EXISTS "PaymentTransactions" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "EntityType" VARCHAR(50) NOT NULL DEFAULT 'ROUTE_RESERVATION',
    "EntityCode" VARCHAR(50) NOT NULL,
    "Provider" VARCHAR(50) NOT NULL, -- 'STRIPE', 'OPENPAY', 'MERCADOPAGO'
    "PaymentMethod" VARCHAR(50) NOT NULL DEFAULT 'CARD',
    "Amount" NUMERIC(10, 2) NOT NULL,
    "Currency" VARCHAR(10) NOT NULL DEFAULT 'mxn',
    "Status" VARCHAR(50) NOT NULL DEFAULT 'PENDING', -- 'PENDING', 'SUCCEEDED', 'FAILED', 'EXPIRED', 'CANCELLED'
    "PaymentIntentId" VARCHAR(255),
    "SessionId" VARCHAR(255),
    "Clabe" VARCHAR(50),
    "BankName" VARCHAR(100),
    "ReceiptUrl" TEXT,
    "HostedUrl" TEXT,
    "RawPayload" TEXT,
    "ExpiresAt" TIMESTAMPTZ,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" INT NOT NULL DEFAULT 1,
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" INT,
    "UpdateDate" TIMESTAMP
);

CREATE INDEX IF NOT EXISTS "IDX_PaymentTransactions_Entity" ON "PaymentTransactions" ("EntityType", "EntityCode");
CREATE INDEX IF NOT EXISTS "IDX_PaymentTransactions_Status" ON "PaymentTransactions" ("Status");
CREATE INDEX IF NOT EXISTS "IDX_PaymentTransactions_ExpiresAt_Pending" ON "PaymentTransactions" ("ExpiresAt") WHERE "Status" = 'PENDING' AND "IsActive" = true;
CREATE INDEX IF NOT EXISTS "IDX_PaymentTransactions_PaymentIntentId" ON "PaymentTransactions" ("PaymentIntentId");
CREATE INDEX IF NOT EXISTS "IDX_PaymentTransactions_Clabe" ON "PaymentTransactions" ("Clabe");

-- 9. OpenPaySettings
CREATE TABLE IF NOT EXISTS "OpenPaySettings" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL DEFAULT 'DEFAULT' UNIQUE,
    "Name" VARCHAR(255) NOT NULL DEFAULT 'OpenPay Settings',
    "MerchantId" VARCHAR(255),
    "PublicKey" TEXT,
    "PrivateKey" TEXT,
    "WebhookSecretKey" TEXT,
    "ApiBaseUrl" VARCHAR(255) NOT NULL DEFAULT 'https://sandbox-api.openpay.mx/v1',
    "IsTestMode" BOOLEAN NOT NULL DEFAULT true,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" INT NOT NULL DEFAULT 1,
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" INT,
    "UpdateDate" TIMESTAMP
);

INSERT INTO "OpenPaySettings" ("Code", "Name", "IsTestMode", "IsActive", "CreatedBy") VALUES
('DEFAULT', 'OpenPay Settings', true, true, 1)
ON CONFLICT ("Code") DO NOTHING;

-- 10. MercadoPagoSettings
CREATE TABLE IF NOT EXISTS "MercadoPagoSettings" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL DEFAULT 'DEFAULT' UNIQUE,
    "Name" VARCHAR(255) NOT NULL DEFAULT 'Mercado Pago Settings',
    "PublicKey" TEXT,
    "AccessToken" TEXT,
    "WebhookSecretKey" TEXT,
    "ApiBaseUrl" VARCHAR(255) NOT NULL DEFAULT 'https://api.mercadopago.com',
    "IsTestMode" BOOLEAN NOT NULL DEFAULT true,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" INT NOT NULL DEFAULT 1,
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" INT,
    "UpdateDate" TIMESTAMP
);

INSERT INTO "MercadoPagoSettings" ("Code", "Name", "IsTestMode", "IsActive", "CreatedBy") VALUES
('DEFAULT', 'Mercado Pago Settings', true, true, 1)
ON CONFLICT ("Code") DO NOTHING;

-- 11. StripeSettings
CREATE TABLE IF NOT EXISTS "StripeSettings" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL DEFAULT 'DEFAULT' UNIQUE,
    "Name" VARCHAR(255) NOT NULL DEFAULT 'Stripe Payment Settings',
    "PublishableKey" TEXT,
    "SecretKey" TEXT,
    "WebhookSecretKey" TEXT,
    "ApiBaseUrl" VARCHAR(255) NOT NULL DEFAULT 'https://api.stripe.com/v1',
    "CheckoutEndpoint" VARCHAR(255) NOT NULL DEFAULT '/checkout/sessions',
    "WebhookPath" VARCHAR(255) NOT NULL DEFAULT '/api/v1/stripe/webhook',
    "Currency" VARCHAR(10) NOT NULL DEFAULT 'mxn',
    "IsTestMode" BOOLEAN NOT NULL DEFAULT true,
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" INT NOT NULL DEFAULT 1,
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" INT,
    "UpdateDate" TIMESTAMP
);

-- 12. WhatsAppSettings (Meta Cloud API)
CREATE TABLE IF NOT EXISTS "WhatsAppSettings" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE DEFAULT 'DEFAULT',
    "Name" VARCHAR(255) NOT NULL DEFAULT 'MetaConfig',
    "PhoneId" VARCHAR(255),
    "ApiToken" TEXT,
    "BusinessAccountId" VARCHAR(255),
    "WebhookVerifyToken" TEXT,
    "ApiVersion" VARCHAR(50) NOT NULL DEFAULT 'v24.0',
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" INT NOT NULL DEFAULT 1,
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" INT,
    "UpdateDate" TIMESTAMP
);

INSERT INTO "WhatsAppSettings" ("Code", "Name", "PhoneId", "ApiToken", "BusinessAccountId", "WebhookVerifyToken", "ApiVersion", "IsActive", "CreatedBy") VALUES
('DEFAULT', 'MetaConfig', '', '', '', '', 'v24.0', true, 1)
ON CONFLICT ("Code") DO NOTHING;

-- 13. WhatsAppConversations
CREATE TABLE IF NOT EXISTS "WhatsAppConversations" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "CustomerPhone" VARCHAR(50) NOT NULL,
    "CustomerName" VARCHAR(255),
    "LastMessageText" TEXT,
    "LastMessageDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "UnreadCount" INT NOT NULL DEFAULT 0,
    "Status" VARCHAR(50) NOT NULL DEFAULT 'OPEN',
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" INT NOT NULL DEFAULT 1,
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" INT,
    "UpdateDate" TIMESTAMP
);

CREATE INDEX IF NOT EXISTS "IDX_WhatsAppConversations_CustomerPhone" ON "WhatsAppConversations" ("CustomerPhone");

-- 14. WhatsAppMessages
CREATE TABLE IF NOT EXISTS "WhatsAppMessages" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(255),
    "ConversationEntry" INT NOT NULL,
    "SenderType" VARCHAR(50) NOT NULL DEFAULT 'OUTBOUND',
    "MessageType" VARCHAR(50) NOT NULL DEFAULT 'TEXT',
    "TemplateName" VARCHAR(255),
    "Content" TEXT,
    "MediaUrl" TEXT,
    "Status" VARCHAR(50) NOT NULL DEFAULT 'SENT',
    "ResponsePayload" TEXT,
    "ErrorMessage" TEXT,
    "SentBy" INT NOT NULL DEFAULT 1,
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS "IDX_WhatsAppMessages_ConversationEntry" ON "WhatsAppMessages" ("ConversationEntry");
CREATE INDEX IF NOT EXISTS "IDX_WhatsAppMessages_Code" ON "WhatsAppMessages" ("Code");

-- 15. WhatsAppTemplates (Plantillas Oficiales de Meta Cloud API)
CREATE TABLE IF NOT EXISTS "WhatsAppTemplates" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "TemplateName" VARCHAR(100) NOT NULL,
    "Language" VARCHAR(10) NOT NULL DEFAULT 'es_MX',
    "Category" VARCHAR(50) NOT NULL DEFAULT 'UTILITY', -- 'MARKETING', 'UTILITY', 'AUTHENTICATION'
    "HeaderType" VARCHAR(20) NOT NULL DEFAULT 'NONE',
    "BodyTemplate" TEXT NOT NULL,
    "FooterText" VARCHAR(255),
    "ButtonsJson" TEXT,
    "Status" VARCHAR(50) NOT NULL DEFAULT 'APPROVED',
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" INT NOT NULL DEFAULT 1,
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" INT,
    "UpdateDate" TIMESTAMP
);

CREATE INDEX IF NOT EXISTS "IDX_WhatsAppTemplates_TemplateName" ON "WhatsAppTemplates" ("TemplateName");

-- 16. CustomMessages (Plantillas Internas Dinámicas de OmniSphere / OmniRoute)
CREATE TABLE IF NOT EXISTS "CustomMessages" (
    "Entry" SERIAL PRIMARY KEY,
    "Code" VARCHAR(50) NOT NULL UNIQUE,
    "Title" VARCHAR(255) NOT NULL,
    "MessageType" VARCHAR(50) NOT NULL DEFAULT 'TEXT',
    "HeaderType" VARCHAR(20) NOT NULL DEFAULT 'NONE',
    "HeaderContent" TEXT,
    "BodyTemplate" TEXT NOT NULL,
    "FooterText" VARCHAR(255),
    "IsActive" BOOLEAN NOT NULL DEFAULT true,
    "CreatedBy" INT NOT NULL DEFAULT 1,
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    "LastUpdatedBy" INT,
    "UpdateDate" TIMESTAMP
);

CREATE INDEX IF NOT EXISTS "IDX_CustomMessages_Code" ON "CustomMessages" ("Code");

-- 17. CustomMessageParameters (Parámetros y Tipos de Datos de Plantillas Internas)
CREATE TABLE IF NOT EXISTS "CustomMessageParameters" (
    "Entry" SERIAL PRIMARY KEY,
    "MessageCode" VARCHAR(50) NOT NULL,
    "ParamKey" VARCHAR(50) NOT NULL,
    "ParamName" VARCHAR(100) NOT NULL,
    "DataType" VARCHAR(30) NOT NULL DEFAULT 'STRING',
    "DefaultValue" VARCHAR(255),
    "IsRequired" BOOLEAN NOT NULL DEFAULT true,
    "Description" TEXT,
    "SortOrder" INT NOT NULL DEFAULT 1,
    "CreatedBy" INT NOT NULL DEFAULT 1,
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT "FK_MsgParams_MessageCode" FOREIGN KEY ("MessageCode") REFERENCES "CustomMessages"("Code") ON DELETE CASCADE,
    CONSTRAINT "UQ_MsgParams_Key" UNIQUE ("MessageCode", "ParamKey")
);

CREATE INDEX IF NOT EXISTS "IDX_CustomMessageParameters_MessageCode" ON "CustomMessageParameters" ("MessageCode");

-- 18. CustomButtons (Botones Interactivos de Plantillas Internas)
CREATE TABLE IF NOT EXISTS "CustomButtons" (
    "Entry" SERIAL PRIMARY KEY,
    "MessageEntry" INT NOT NULL REFERENCES "CustomMessages"("Entry") ON DELETE CASCADE,
    "ButtonId" VARCHAR(50) NOT NULL,
    "Title" VARCHAR(50) NOT NULL,
    "ActionType" VARCHAR(50) NOT NULL DEFAULT 'TRIGGER_MESSAGE',
    "ActionPayload" TEXT,
    "SortOrder" INT NOT NULL DEFAULT 1,
    "CreatedBy" INT NOT NULL DEFAULT 1,
    "CreateDate" TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS "IDX_CustomButtons_MessageEntry" ON "CustomButtons" ("MessageEntry");

-- Seed Default Internal Custom Templates
DELETE FROM "CustomButtons" WHERE "MessageEntry" IN (SELECT "Entry" FROM "CustomMessages" WHERE "Code" IN ('TPL_WELCOME_WITH_RESERVATION', 'TPL_WELCOME_PROMPT', 'TPL_RESERVATION_DETAILS', 'TPL_NOT_FOUND_ERROR', 'TPL_CARD_PAYMENT_SUCCESS', 'TPL_CARD_PAYMENT_FAILED', 'TPL_BANK_TRANSFER_INFO', 'TPL_SPEI_PAYMENT_RECEIVED', 'TPL_PAYMENT_REJECTED', 'TPL_TRANSFER_REJECTED'));
DELETE FROM "CustomMessageParameters" WHERE "MessageCode" IN ('TPL_WELCOME_WITH_RESERVATION', 'TPL_WELCOME_PROMPT', 'TPL_RESERVATION_DETAILS', 'TPL_NOT_FOUND_ERROR', 'TPL_CARD_PAYMENT_SUCCESS', 'TPL_CARD_PAYMENT_FAILED', 'TPL_BANK_TRANSFER_INFO', 'TPL_SPEI_PAYMENT_RECEIVED', 'TPL_PAYMENT_REJECTED', 'TPL_TRANSFER_REJECTED');
DELETE FROM "CustomMessages" WHERE "Code" IN ('TPL_WELCOME_WITH_RESERVATION', 'TPL_WELCOME_PROMPT', 'TPL_RESERVATION_DETAILS', 'TPL_NOT_FOUND_ERROR', 'TPL_CARD_PAYMENT_SUCCESS', 'TPL_CARD_PAYMENT_FAILED', 'TPL_BANK_TRANSFER_INFO', 'TPL_SPEI_PAYMENT_RECEIVED', 'TPL_PAYMENT_REJECTED', 'TPL_TRANSFER_REJECTED');

INSERT INTO "CustomMessages" ("Code", "Title", "MessageType", "BodyTemplate", "CreatedBy") VALUES
('TPL_WELCOME_WITH_RESERVATION', 'Bienvenida a Cliente Reconocido', 'INTERACTIVE_BUTTON', 
'¡Hola {nombre_registrado}! 👋 Qué gusto saludarte.

Vemos que tienes una reservación activa para:
🎪 *{evento}*
🎟️ Folio: *{folio}*
📍 Origen: *{parada_inicial}*
⏰ Salida: *{hora_salida}*
💳 Estatus: *{estatus}*

¿Deseas consultar los detalles completos de tu viaje o necesitas ayuda adicional?', 1),

('TPL_WELCOME_PROMPT', 'Bienvenida a Cliente No Registrado', 'TEXT',
'¡Hola {nombre_cliente}! 👋 Bienvenido al asistente de {empresa} 🚌✨

No encontramos reservaciones activas vinculadas a este número de WhatsApp.

Si compraste con otro número o deseas consultar tu boleto:
🎟️ Escribe el folio de tu boleto (ejemplo: *{ejemplo_folio}*), o
📱 Los *10 dígitos* del número celular con el que te registraste.', 1),

('TPL_RESERVATION_DETAILS', 'Detalle Completo de Reservación', 'TEXT',
'📋 *DETALLES DE TU RESERVACIÓN* 🚌✨

🎟️ *Folio:* {folio}
📱 *Teléfono:* {numero_registrado}
👤 *Pasajero:* {nombre_registrado}
💺 *Asientos:* {numero_asientos}
🎪 *Evento:* {evento}
📍 *Origen:* {parada_inicial}
🏁 *Destino:* {destino}
⏰ *Hora de Salida:* {hora_salida}
🏁 *Hora Estimada de Llegada:* {hora_llegada}
💰 *Importe Pagado:* {importe_pagado}
💳 *Forma de Pago:* {detalle_pago}

✅ *Estatus:* {estatus}

¡Te esperamos puntualmente en tu punto de abordaje! 🎒', 1),

('TPL_NOT_FOUND_ERROR', 'Reservación No Encontrada', 'TEXT',
'Lo sentimos {nombre_cliente}, no pudimos encontrar ninguna reservación activa con los datos ingresados: "{dato_ingresado}". 🔍

Por favor verifica que:
• El folio inicie con *RSV* (ejemplo: *{ejemplo_folio}*), o
• Hayas escrito los *10 dígitos* del número celular registrado.

Si necesitas ayuda personalizada, nuestro equipo con gusto te atenderá.', 1),

('TPL_CARD_PAYMENT_SUCCESS', 'Pago con Tarjeta Exitoso', 'TEXT',
'¡Tu pago con tarjeta ha sido exitoso! 🎉💳

Hola {nombre_registrado}, confirmamos la acreditación de tu pago:
🎟️ *Folio:* {folio}
🎪 *Evento:* {evento}
💺 *Asientos:* {numero_asientos}
💰 *Monto Pagado:* {monto_pagado}
💳 *Tarjeta:* {marca_tarjeta} terminación •••• {ultimos_4_digitos}
📍 *Salida:* {parada_inicial} ({hora_salida})

✅ *Estatus:* CONFIRMADO Y PAGADO

¡Tu lugar está 100% asegurado! Nos vemos en el evento. 🚌✨', 1),

('TPL_CARD_PAYMENT_FAILED', 'Pago con Tarjeta Declinado', 'TEXT',
'Aviso sobre el pago de tu reservación ⚠️💳

Hola {nombre_registrado}, no pudimos procesar el cobro con tu tarjeta para el folio *{folio}*:
💰 Monto: *{monto_pagado}*
Motivo: {motivo_fallo}

Para no perder tus lugares, por favor intenta con otra tarjeta o cambia tu forma de pago a Transferencia bancaria.', 1),

('TPL_TRANSFER_INSTRUCTIONS', 'Instrucciones de Transferencia Bancaria', 'TEXT',
'Instrucciones para Pago por Transferencia 🏦📋

Hola {nombre_registrado}, para completar tu reservación *{folio}*, realiza tu transferencia con los siguientes datos:

🏦 *Banco:* {banco}
👤 *Beneficiario:* {beneficiario}
🔢 *CLABE:* `{clabe}`
💰 *Monto Exacto:* *{monto_a_pagar}*
📝 *Concepto / Referencia:* `{folio}`

📸 *Importante:* Una vez realizada, envía la captura de tu comprobante por este chat para validar y asegurar tus lugares.', 1),

('TPL_TRANSFER_APPROVED', 'Transferencia Acreditada', 'TEXT',
'¡Tu transferencia ha sido verificada con éxito! 🎉✅

Hola {nombre_registrado}, validamos tu comprobante de pago:
🎟️ *Folio:* {folio}
🎪 *Evento:* {evento}
💺 *Asientos:* {numero_asientos}
💰 *Monto Acreditado:* {monto_pagado}
📍 *Salida:* {parada_inicial} ({hora_salida})

✅ *Estatus:* CONFIRMADO Y PAGADO

¡Tu viaje está confirmado! Te esperamos en tu punto de abordaje. 🚌✨', 1),

('TPL_TRANSFER_REJECTED', 'Comprobante de Transferencia Rechazado', 'TEXT',
'Aviso sobre tu comprobante de transferencia ⚠️📄

Hola {nombre_registrado}, tuvimos un inconveniente al validar tu comprobante para el folio *{folio}*:
Motivo: {motivo_rechazo}

Por favor envía un nuevo comprobante legible o comunícate por este chat para asistirte.', 1)
ON CONFLICT ("Code") DO NOTHING;

-- Seed Buttons for TPL_WELCOME_WITH_RESERVATION
INSERT INTO "CustomButtons" ("MessageEntry", "ButtonId", "Title", "ActionType", "SortOrder", "CreatedBy")
SELECT m."Entry", 'BTN_DETAILS_{folio}', 'Ver Detalles', 'TRIGGER_MESSAGE', 1, 1
FROM "CustomMessages" m WHERE m."Code" = 'TPL_WELCOME_WITH_RESERVATION'
ON CONFLICT DO NOTHING;

-- Seed Parameters for TPL_WELCOME_WITH_RESERVATION
INSERT INTO "CustomMessageParameters" ("MessageCode", "ParamKey", "ParamName", "DataType", "DefaultValue", "IsRequired", "SortOrder") VALUES
('TPL_WELCOME_WITH_RESERVATION', 'nombre_registrado', 'Nombre del Pasajero', 'STRING', 'estimado(a) cliente', true, 1),
('TPL_WELCOME_WITH_RESERVATION', 'evento', 'Nombre del Evento', 'STRING', 'Evento General', true, 2),
('TPL_WELCOME_WITH_RESERVATION', 'folio', 'Folio de Reservación', 'STRING', 'RSV000000', true, 3),
('TPL_WELCOME_WITH_RESERVATION', 'parada_inicial', 'Parada de Salida', 'STRING', 'Punto de Abordaje', true, 4),
('TPL_WELCOME_WITH_RESERVATION', 'hora_salida', 'Hora de Salida', 'TIME', 'Por confirmar', true, 5),
('TPL_WELCOME_WITH_RESERVATION', 'estatus', 'Estatus de la Reserva', 'STRING', 'CONFIRMADO', true, 6)
ON CONFLICT ("MessageCode", "ParamKey") DO NOTHING;

-- Seed Parameters for TPL_WELCOME_PROMPT
INSERT INTO "CustomMessageParameters" ("MessageCode", "ParamKey", "ParamName", "DataType", "DefaultValue", "IsRequired", "SortOrder") VALUES
('TPL_WELCOME_PROMPT', 'nombre_cliente', 'Nombre del Perfil', 'STRING', 'estimado(a) cliente', false, 1),
('TPL_WELCOME_PROMPT', 'empresa', 'Nombre de Empresa', 'STRING', 'OmniRoute', true, 2),
('TPL_WELCOME_PROMPT', 'ejemplo_folio', 'Ejemplo de Folio', 'STRING', 'RSV000001', true, 3)
ON CONFLICT ("MessageCode", "ParamKey") DO NOTHING;

-- Seed Parameters for TPL_RESERVATION_DETAILS
INSERT INTO "CustomMessageParameters" ("MessageCode", "ParamKey", "ParamName", "DataType", "DefaultValue", "IsRequired", "SortOrder") VALUES
('TPL_RESERVATION_DETAILS', 'folio', 'Folio de Reserva', 'STRING', 'RSV000000', true, 1),
('TPL_RESERVATION_DETAILS', 'numero_registrado', 'Teléfono Registrado', 'STRING', '', true, 2),
('TPL_RESERVATION_DETAILS', 'nombre_registrado', 'Nombre del Pasajero', 'STRING', 'Pasajero', true, 3),
('TPL_RESERVATION_DETAILS', 'numero_asientos', 'Número de Asientos', 'INTEGER', '1', true, 4),
('TPL_RESERVATION_DETAILS', 'evento', 'Nombre del Evento', 'STRING', 'Evento', true, 5),
('TPL_RESERVATION_DETAILS', 'parada_inicial', 'Parada de Salida', 'STRING', 'Origen', true, 6),
('TPL_RESERVATION_DETAILS', 'destino', 'Destino Final', 'STRING', 'Destino', true, 7),
('TPL_RESERVATION_DETAILS', 'hora_salida', 'Hora de Salida', 'TIME', '00:00', true, 8),
('TPL_RESERVATION_DETAILS', 'hora_llegada', 'Hora Estimada de Llegada', 'TIME', '00:00', false, 9),
('TPL_RESERVATION_DETAILS', 'importe_pagado', 'Monto Pagado', 'CURRENCY', '$0.00 MXN', true, 10),
('TPL_RESERVATION_DETAILS', 'detalle_pago', 'Detalle de Pago', 'STRING', 'Pago General', true, 11),
('TPL_RESERVATION_DETAILS', 'estatus', 'Estatus de la Reserva', 'STRING', 'CONFIRMADO', true, 12)
ON CONFLICT ("MessageCode", "ParamKey") DO NOTHING;

-- Seed Parameters for TPL_NOT_FOUND_ERROR
INSERT INTO "CustomMessageParameters" ("MessageCode", "ParamKey", "ParamName", "DataType", "DefaultValue", "IsRequired", "SortOrder") VALUES
('TPL_NOT_FOUND_ERROR', 'nombre_cliente', 'Nombre del Perfil', 'STRING', 'estimado(a) cliente', false, 1),
('TPL_NOT_FOUND_ERROR', 'dato_ingresado', 'Dato Ingresado', 'STRING', '', true, 2),
('TPL_NOT_FOUND_ERROR', 'ejemplo_folio', 'Ejemplo de Folio', 'STRING', 'RSV000001', true, 3)
ON CONFLICT ("MessageCode", "ParamKey") DO NOTHING;

-- Seed Parameters for TPL_CARD_PAYMENT_SUCCESS
INSERT INTO "CustomMessageParameters" ("MessageCode", "ParamKey", "ParamName", "DataType", "DefaultValue", "IsRequired", "SortOrder") VALUES
('TPL_CARD_PAYMENT_SUCCESS', 'nombre_registrado', 'Nombre del Pasajero', 'STRING', 'Pasajero', true, 1),
('TPL_CARD_PAYMENT_SUCCESS', 'folio', 'Folio de Reserva', 'STRING', '', true, 2),
('TPL_CARD_PAYMENT_SUCCESS', 'evento', 'Nombre del Evento', 'STRING', '', true, 3),
('TPL_CARD_PAYMENT_SUCCESS', 'numero_asientos', 'Número de Asientos', 'INTEGER', '1', true, 4),
('TPL_CARD_PAYMENT_SUCCESS', 'monto_pagado', 'Monto Pagado', 'CURRENCY', '$0.00 MXN', true, 5),
('TPL_CARD_PAYMENT_SUCCESS', 'marca_tarjeta', 'Marca de Tarjeta', 'STRING', 'Tarjeta', true, 6),
('TPL_CARD_PAYMENT_SUCCESS', 'ultimos_4_digitos', 'Últimos 4 Dígitos', 'MASKED_CARD', '••••', true, 7),
('TPL_CARD_PAYMENT_SUCCESS', 'parada_inicial', 'Parada de Salida', 'STRING', '', true, 8),
('TPL_CARD_PAYMENT_SUCCESS', 'hora_salida', 'Hora de Salida', 'TIME', '', true, 9)
ON CONFLICT ("MessageCode", "ParamKey") DO NOTHING;

-- Seed Parameters for TPL_CARD_PAYMENT_FAILED
INSERT INTO "CustomMessageParameters" ("MessageCode", "ParamKey", "ParamName", "DataType", "DefaultValue", "IsRequired", "SortOrder") VALUES
('TPL_CARD_PAYMENT_FAILED', 'nombre_registrado', 'Nombre del Pasajero', 'STRING', 'Pasajero', true, 1),
('TPL_CARD_PAYMENT_FAILED', 'folio', 'Folio de Reserva', 'STRING', '', true, 2),
('TPL_CARD_PAYMENT_FAILED', 'monto_pagado', 'Monto Intentado', 'CURRENCY', '$0.00 MXN', true, 3),
('TPL_CARD_PAYMENT_FAILED', 'motivo_fallo', 'Motivo del Fallo', 'STRING', 'Fondos insuficientes o declinada', true, 4)
ON CONFLICT ("MessageCode", "ParamKey") DO NOTHING;

-- Seed Parameters for TPL_TRANSFER_INSTRUCTIONS
INSERT INTO "CustomMessageParameters" ("MessageCode", "ParamKey", "ParamName", "DataType", "DefaultValue", "IsRequired", "SortOrder") VALUES
('TPL_TRANSFER_INSTRUCTIONS', 'nombre_registrado', 'Nombre del Pasajero', 'STRING', 'Pasajero', true, 1),
('TPL_TRANSFER_INSTRUCTIONS', 'folio', 'Folio de Reserva', 'STRING', '', true, 2),
('TPL_TRANSFER_INSTRUCTIONS', 'banco', 'Banco Destino', 'STRING', 'BBVA', true, 3),
('TPL_TRANSFER_INSTRUCTIONS', 'beneficiario', 'Nombre del Beneficiario', 'STRING', 'OmniRoute Transportes S.A. de C.V.', true, 4),
('TPL_TRANSFER_INSTRUCTIONS', 'clabe', 'CLABE Interbancaria', 'STRING', '012180001234567890', true, 5),
('TPL_TRANSFER_INSTRUCTIONS', 'monto_a_pagar', 'Monto a Pagar', 'CURRENCY', '$0.00 MXN', true, 6)
ON CONFLICT ("MessageCode", "ParamKey") DO NOTHING;

-- Seed Parameters for TPL_TRANSFER_APPROVED
INSERT INTO "CustomMessageParameters" ("MessageCode", "ParamKey", "ParamName", "DataType", "DefaultValue", "IsRequired", "SortOrder") VALUES
('TPL_TRANSFER_APPROVED', 'nombre_registrado', 'Nombre del Pasajero', 'STRING', 'Pasajero', true, 1),
('TPL_TRANSFER_APPROVED', 'folio', 'Folio de Reserva', 'STRING', '', true, 2),
('TPL_TRANSFER_APPROVED', 'evento', 'Nombre del Evento', 'STRING', '', true, 3),
('TPL_TRANSFER_APPROVED', 'numero_asientos', 'Número de Asientos', 'INTEGER', '1', true, 4),
('TPL_TRANSFER_APPROVED', 'monto_pagado', 'Monto Acreditado', 'CURRENCY', '$0.00 MXN', true, 5),
('TPL_TRANSFER_APPROVED', 'parada_inicial', 'Parada de Salida', 'STRING', '', true, 6),
('TPL_TRANSFER_APPROVED', 'hora_salida', 'Hora de Salida', 'TIME', '', true, 7)
ON CONFLICT ("MessageCode", "ParamKey") DO NOTHING;

-- Seed Parameters for TPL_TRANSFER_REJECTED
INSERT INTO "CustomMessageParameters" ("MessageCode", "ParamKey", "ParamName", "DataType", "DefaultValue", "IsRequired", "SortOrder") VALUES
('TPL_TRANSFER_REJECTED', 'nombre_registrado', 'Nombre del Pasajero', 'STRING', 'Pasajero', true, 1),
('TPL_TRANSFER_REJECTED', 'folio', 'Folio de Reserva', 'STRING', '', true, 2),
('TPL_TRANSFER_REJECTED', 'motivo_rechazo', 'Motivo del Rechazo', 'STRING', 'Comprobante ilegible o monto incompleto', true, 3)
ON CONFLICT ("MessageCode", "ParamKey") DO NOTHING;

